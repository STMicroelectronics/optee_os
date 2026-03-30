/*
 * Arm SCP/MCP Software
 * Copyright (c) 2024, Arm Limited and Contributors. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <mod_power_domain.h>
#include <mod_scmi.h>
#include <mod_scmi_power_domain_req.h>
#include <mod_timer.h>

#include <fwk_assert.h>
#include <fwk_core.h>
#include <fwk_event.h>
#include <fwk_id.h>
#include <fwk_interrupt.h>
#include <fwk_log.h>
#include <fwk_macros.h>
#include <fwk_mm.h>
#include <fwk_module.h>
#include <fwk_module_idx.h>
#include <fwk_status.h>
#include <fwk_string.h>

#include <stdbool.h>
#include <stdint.h>

/* Element context */
struct scmi_power_domain_req_dev_ctx {
    /* Element configuration data pointer */
    const struct scmi_power_domain_req_dev_config *config;

    /* PD State - collated after Set State command responses */
    uint32_t current_state;

    /* PD State - collated after Set State command called */
    uint32_t requested_state;

    /* Whether or not the response has been received */
    bool is_free;
};

/* Module context */
struct scmi_power_domain_req_ctx {
    /* SCMI power domain requester element context table */
    struct scmi_power_domain_req_dev_ctx *dev_ctx_table;

    /* token to track the sent messages */
    uint8_t token;

    /* Number of elements */
    unsigned int dev_count;

    /* SCMI send message API */
    const struct mod_scmi_from_protocol_req_api *scmi_api;

    /* Power Domain driver input API */
    const struct mod_pd_driver_input_api *pd_driver_input_api;

#ifdef BUILD_HAS_MOD_TIMER
    /* Timer alarm API */
    const struct mod_timer_alarm_api *alarm_api;
#endif
};

static int scmi_power_domain_req_state_set_resp_handler(
    fwk_id_t service_id,
    const void *payload,
    size_t payload_size);

/*! Identifier of the system power req API */
const fwk_id_t mod_power_domain_req_api_id = FWK_ID_API_INIT(
    FWK_MODULE_IDX_SCMI_POWER_DOMAIN_REQ,
    MOD_POW_DOMAIN_REQ_API_IDX_REQ);

/*! Identifier of the system power req API */
const fwk_id_t mod_power_domain_req_scmi_api_id = FWK_ID_API_INIT(
    FWK_MODULE_IDX_SCMI_POWER_DOMAIN_REQ,
    MOD_POW_DOMAIN_REQ_API_IDX_SCMI_REQ);

/*
 * Internal variables.
 */
static struct scmi_power_domain_req_ctx mod_ctx;

/*!
 * \brief SCMI Power Domain Protocol Message IDs
 */

enum scmi_pd_req_command_id {
    /*
     * SCMI Command ID of the Power Domain command
     * implemented in this module.
     */
    MOD_SCMI_POWER_DOMAIN_REQ_STATE_SET = 0x004,
    MOD_SCMI_POWER_DOMAIN_REQ_COMMAND_COUNT,
};

static int (*handler_table[MOD_SCMI_POWER_DOMAIN_REQ_COMMAND_COUNT])(
    fwk_id_t,
    const void *,
    size_t) = {
    [MOD_SCMI_POWER_DOMAIN_REQ_STATE_SET] =
        scmi_power_domain_req_state_set_resp_handler,
};

static const unsigned int
    payload_size_table[MOD_SCMI_POWER_DOMAIN_REQ_COMMAND_COUNT] = {
        [MOD_SCMI_POWER_DOMAIN_REQ_STATE_SET] =
            (unsigned int)sizeof(struct scmi_pd_req_power_state_set_p2a),
    };

static_assert(
    FWK_ARRAY_SIZE(handler_table) == FWK_ARRAY_SIZE(payload_size_table),
    "[SCMI] Power Domain Req protocol table sizes not "
    "consistent");

static inline struct scmi_power_domain_req_dev_ctx *get_dev_ctx(fwk_id_t id)
{
    return &mod_ctx.dev_ctx_table[fwk_id_get_element_idx(id)];
}

static bool try_get_element_idx_from_service(
    fwk_id_t service_id,
    uint32_t *element_idx)
{
    unsigned int i;

    for (i = 0; i < mod_ctx.dev_count; i++) {
        if (fwk_id_is_equal(
                service_id, mod_ctx.dev_ctx_table[i].config->service_id)) {
            *element_idx = i;
            return true;
        }
    }
    return false;
}

/*
 * SCMI module -> SCMI power domain requester module interface
 */
static int scmi_power_domain_req_get_scmi_protocol_id(
    fwk_id_t protocol_id,
    uint8_t *scmi_protocol_id)
{
    *scmi_protocol_id = (uint8_t)MOD_SCMI_PROTOCOL_ID_POWER_DOMAIN;

    return FWK_SUCCESS;
}

/*
 * Power Domain Requester Response handlers
 */
static int scmi_power_domain_req_message_handler(
    fwk_id_t protocol_id,
    fwk_id_t service_id,
    const uint32_t *payload,
    size_t payload_size,
    unsigned int message_id)
{
    int ret_status, alt_status;

    fwk_assert(payload != NULL);

    if (message_id >= FWK_ARRAY_SIZE(handler_table)) {
        return FWK_E_RANGE;
    }

    if (payload_size != payload_size_table[message_id]) {
        return FWK_E_PARAM;
    }

    if (handler_table[message_id] == NULL) {
        return FWK_E_PARAM;
    }

    ret_status = handler_table[message_id](service_id, payload, payload_size);

    if (ret_status == FWK_SUCCESS) {
        alt_status = mod_ctx.scmi_api->response_message_handler(service_id);
    }

    return (ret_status != FWK_SUCCESS) ? ret_status : alt_status;
}

static struct mod_scmi_to_protocol_api
    scmi_power_domain_req_scmi_to_protocol_api = {
        .get_scmi_protocol_id = scmi_power_domain_req_get_scmi_protocol_id,
        .message_handler = scmi_power_domain_req_message_handler,
    };

/*
 * This is the State set response handler.
 */
static int scmi_power_domain_req_state_set_resp_handler(
    fwk_id_t service_id,
    const void *payload,
    size_t payload_size)
{
    int status;
    struct scmi_pd_req_power_state_set_p2a *return_value;
    uint32_t element_idx;
    struct scmi_power_domain_req_dev_ctx *dev_ctx;

    if (try_get_element_idx_from_service(service_id, &element_idx)) {
        dev_ctx = &(mod_ctx.dev_ctx_table[element_idx]);
    } else {
        return FWK_E_DATA;
    }

    dev_ctx->is_free = true;

#ifdef BUILD_HAS_MOD_TIMER
    /* Disable the timer as response has been received */
    status = mod_ctx.alarm_api->stop(dev_ctx->config->timer_id);

    fwk_assert(status == FWK_SUCCESS);
#endif

    if (payload_size != sizeof(struct scmi_pd_req_power_state_set_p2a)) {
        return FWK_E_DATA;
    }

    return_value = (struct scmi_pd_req_power_state_set_p2a *)payload;

    if (return_value->status == SCMI_SUCCESS) {
        if (dev_ctx->current_state != dev_ctx->requested_state) {
            dev_ctx->current_state = dev_ctx->requested_state;
        }

        /* Report to the power domain module the transition completion */
        status = mod_ctx.pd_driver_input_api->report_power_state_transition(
            dev_ctx->config->pd_element_id, dev_ctx->requested_state);

        if (status != FWK_SUCCESS) {
            return status;
        }
    }

    return FWK_SUCCESS;
}

#ifdef BUILD_HAS_MOD_TIMER
/*
 * Callback for the response timeout, it is called when no response from
 * SCMI is received for a specific timeout.
 */
static void response_timeout_callback(uintptr_t param)
{
    uint32_t element_idx = (unsigned int)param;

    struct scmi_power_domain_req_dev_ctx *dev_ctx =
        &(mod_ctx.dev_ctx_table[element_idx]);

    /* Set the channel to free and ignore the requested state */
    dev_ctx->is_free = true;
    dev_ctx->requested_state = dev_ctx->current_state;
}
#endif

/* Power Domain driver APIs */

/*
 * API to send scmi set command with a requested state for external pd
 */
static int scmi_pd_req_set_state(fwk_id_t pd_id, unsigned int power_state)
{
    int status;
    struct scmi_pd_req_power_state_set_a2p payload;
    uint32_t element_idx;

    uint8_t scmi_protocol_id = (uint8_t)MOD_SCMI_PROTOCOL_ID_POWER_DOMAIN;
    uint8_t scmi_message_id = (uint8_t)MOD_SCMI_POWER_DOMAIN_REQ_STATE_SET;

    /* Check that the pd_id is valid element */
    if (!fwk_id_is_type(pd_id, FWK_ID_TYPE_ELEMENT)) {
        return FWK_E_DATA;
    }

    element_idx = fwk_id_get_element_idx(pd_id);
    if (element_idx >= mod_ctx.dev_count) {
        return FWK_E_DATA;
    }

    struct scmi_power_domain_req_dev_ctx *dev_ctx =
        &(mod_ctx.dev_ctx_table[element_idx]);

    if (!dev_ctx->is_free) {
        /* Still no response from last request */
        return FWK_E_BUSY;
    }

    /* Prepare the payload and send */
    payload.flags = dev_ctx->config->sync_flag;
    payload.power_state = (uint32_t)power_state;
    payload.domain_id = dev_ctx->config->domain_id;

    status = mod_ctx.scmi_api->scmi_send_message(
        scmi_message_id,
        scmi_protocol_id,
        mod_ctx.token++,
        dev_ctx->config->service_id,
        (const void *)&payload,
        sizeof(payload),
        true);

    if (status != FWK_SUCCESS) {
        return status;
    }

#ifdef BUILD_HAS_MOD_TIMER
    status = mod_ctx.alarm_api->start(
        dev_ctx->config->timer_id,
        dev_ctx->config->response_timeout,
        MOD_TIMER_ALARM_TYPE_ONCE,
        response_timeout_callback,
        (uintptr_t)element_idx);

    if (status == FWK_SUCCESS) {
        dev_ctx->is_free = false;
        dev_ctx->requested_state = (uint32_t)power_state;
    }
#else
    dev_ctx->is_free = false;
    dev_ctx->requested_state = (uint32_t)power_state;
#endif

    return status;
}

/*
 * API to send scmi get command for the external pd current state
 */
static int scmi_pd_req_get_state(fwk_id_t pd_id, unsigned int *state)
{
    uint32_t element_idx;

    if (!fwk_id_is_type(pd_id, FWK_ID_TYPE_ELEMENT)) {
        return FWK_E_DATA;
    }

    element_idx = fwk_id_get_element_idx(pd_id);
    if (element_idx >= mod_ctx.dev_count) {
        return FWK_E_DATA;
    }

    struct scmi_power_domain_req_dev_ctx *dev_ctx =
        &(mod_ctx.dev_ctx_table[element_idx]);

    *state = (unsigned int)dev_ctx->current_state;

    return FWK_SUCCESS;
}

/*
 * API to send scmi reset command for the external pd
 */
static int scmi_pd_req_reset()
{
    /* Not used in power domain requester */
    return FWK_E_SUPPORT;
}

/*
 * API to send scmi shutdown command for the external pd
 */
static int scmi_pd_req_shutdown(
    fwk_id_t id,
    enum mod_pd_system_shutdown shutdown_req)
{
    /* Not used in power domain requester and should not be called */
    return FWK_E_SUPPORT;
}

static const struct mod_pd_driver_api pd_driver = {
    .set_state = scmi_pd_req_set_state,
    .get_state = scmi_pd_req_get_state,
    .reset = scmi_pd_req_reset,
    .shutdown = scmi_pd_req_shutdown,
};

/* Framework APIs*/

static int scmi_power_domain_req_init(
    fwk_id_t module_id,
    unsigned int element_count,
    const void *data)
{
    /* We definitely need elements in this module. */
    if (element_count == 0) {
        return FWK_E_SUPPORT;
    }

    mod_ctx.dev_count = element_count;
    mod_ctx.dev_ctx_table =
        fwk_mm_calloc(element_count, sizeof(mod_ctx.dev_ctx_table[0]));

    return FWK_SUCCESS;
}

static int scmi_power_domain_req_elem_init(
    fwk_id_t element_id,
    unsigned int unused,
    const void *data)

{
    struct scmi_power_domain_req_dev_ctx *dev_ctx;

    dev_ctx = get_dev_ctx(element_id);

    if (data == NULL) {
        return FWK_E_PANIC;
    }

    const struct scmi_power_domain_req_dev_config *elem_config =
        (const struct scmi_power_domain_req_dev_config *)data;

    dev_ctx->config = elem_config;

    /*
     * Configure each element's state at the startup with that set in the
     * config file.
     */
    dev_ctx->current_state = dev_ctx->config->start_state;

    dev_ctx->is_free = true;

#ifdef BUILD_HAS_MOD_TIMER
    /*
     * Timer element is not part of the configuration or timer timeout is
     * equal to 0
     */
    if (!fwk_module_is_valid_sub_element_id(elem_config->timer_id) ||
        elem_config->response_timeout == 0) {
        return FWK_E_SUPPORT;
    }

#endif

    return FWK_SUCCESS;
}

static int scmi_power_domain_req_bind(fwk_id_t id, unsigned int round)
{
    int status = FWK_SUCCESS;

    struct scmi_power_domain_req_dev_ctx *dev_ctx;

    /* Only bind as a module with SCMI */
    if (round == 0) {
        if (fwk_id_is_type(id, FWK_ID_TYPE_MODULE)) {
            /* Bind with SCMI module */
            status = fwk_module_bind(
                FWK_ID_MODULE(FWK_MODULE_IDX_SCMI),
                FWK_ID_API(FWK_MODULE_IDX_SCMI, MOD_SCMI_API_IDX_PROTOCOL_REQ),
                &mod_ctx.scmi_api);
        }
    }
    if (fwk_id_is_type(id, FWK_ID_TYPE_MODULE)) {
        return FWK_SUCCESS;
    }

    dev_ctx = get_dev_ctx(id);

    /* Bind with Power Domain module */
    status = fwk_module_bind(
        dev_ctx->config->pd_element_id,
        FWK_ID_API(FWK_MODULE_IDX_POWER_DOMAIN, MOD_PD_API_IDX_DRIVER_INPUT),
        &mod_ctx.pd_driver_input_api);

#ifdef BUILD_HAS_MOD_TIMER
    /* Bind with Timer module */
    status = fwk_module_bind(
        dev_ctx->config->timer_id,
        FWK_ID_API(FWK_MODULE_IDX_TIMER, MOD_TIMER_API_IDX_TIMER),
        &mod_ctx.alarm_api);
#endif

    return status;
}

static int scmi_power_domain_req_bind_request(
    fwk_id_t source_id,
    fwk_id_t unused,
    fwk_id_t api_id,
    const void **api)
{
    enum mod_power_domain_req_api_idx api_id_type;
    int status;

    api_id_type = (enum mod_power_domain_req_api_idx)fwk_id_get_api_idx(api_id);

    switch (api_id_type) {
    case MOD_POW_DOMAIN_REQ_API_IDX_SCMI_REQ:
        if (fwk_id_is_equal(source_id, FWK_ID_MODULE(FWK_MODULE_IDX_SCMI))) {
            *api = &scmi_power_domain_req_scmi_to_protocol_api;
            status = FWK_SUCCESS;
        } else {
            status = FWK_E_ACCESS;
        }
        break;

    case MOD_POW_DOMAIN_REQ_API_IDX_REQ:
        *api = &pd_driver;
        status = FWK_SUCCESS;
        break;

    default:
        status = FWK_E_SUPPORT;
    }

    return status;
}

static int scmi_power_domain_req_process_event()
{
    int status = FWK_SUCCESS;

    return status;
}

const struct fwk_module module_scmi_power_domain_req = {
    .type = FWK_MODULE_TYPE_PROTOCOL,
    .api_count = (unsigned int)MOD_POW_DOMAIN_REQ_API_IDX_COUNT,
    .event_count = (unsigned int)0,
    .init = scmi_power_domain_req_init,
    .element_init = scmi_power_domain_req_elem_init,
    .bind = scmi_power_domain_req_bind,
    .process_bind_request = scmi_power_domain_req_bind_request,
    .process_event = scmi_power_domain_req_process_event,
};
