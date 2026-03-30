/*
 * Arm SCP/MCP Software
 * Copyright (c) 2025, Arm Limited and Contributors. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <internal/scmi_power_capping_req.h>

#include <mod_scmi.h>
#include <mod_scmi_power_capping_req.h>

#include <interface_power_management.h>

#ifdef BUILD_HAS_MOD_TIMER
#    include <mod_timer.h>
#endif

#include <fwk_assert.h>
#include <fwk_core.h>
#include <fwk_id.h>
#include <fwk_log.h>
#include <fwk_macros.h>
#include <fwk_mm.h>
#include <fwk_module.h>
#include <fwk_module_idx.h>
#include <fwk_status.h>

#include <stdbool.h>
#include <stdint.h>

/* Element Context */
struct scmi_power_capping_req_dev_ctx {
    /* Element configuration data pointer */
    const struct mod_scmi_power_capping_req_dev_config *config;

    /* Whether or not the response has been received */
    bool responded;

    /* For the delayed response */
    uint32_t cookie;

    /* Whether or not the client requested a response */
    bool is_response_requested;
};

/* Module context */
struct scmi_power_capping_req_ctx {
    /* SCMI power capping requester element context table */
    struct scmi_power_capping_req_dev_ctx *dev_ctx_table;

    /* Token to track sent messages */
    uint8_t token;

    /* Number of configured elements */
    uint32_t element_count;

    /* SCMI send message API */
    const struct mod_scmi_from_protocol_req_api *scmi_api;

#ifdef BUILD_HAS_MOD_TIMER
    /* Timer alarm API */
    const struct mod_timer_alarm_api *alarm_api;
#endif
};

static int scmi_power_capping_req_set_cap_handler(
    fwk_id_t service_id,
    const void *payload,
    size_t payload_size);

static int scmi_power_capping_req_message_handler(
    fwk_id_t protocol_id,
    fwk_id_t service_id,
    const uint32_t *payload,
    size_t payload_size,
    unsigned int message_id);

/*
 * Internal variables.
 */
static struct scmi_power_capping_req_ctx mod_ctx;

/*!
 * \brief SCMI Power Capping Protocol Message IDs
 */
enum scmi_pcap_req_command_id {
    /*
     * SCMI Command ID of the Power Capping commands
     * implemented in this module.
     */
    MOD_SCMI_POWER_CAPPING_REQ_CAP_SET = 0x005,
    MOD_SCMI_POWER_CAPPING_REQ_COMMAND_COUNT,
};

static int (*handler_table[MOD_SCMI_POWER_CAPPING_REQ_COMMAND_COUNT])(
    fwk_id_t,
    const void *,
    size_t) = {
    [MOD_SCMI_POWER_CAPPING_REQ_CAP_SET] =
        scmi_power_capping_req_set_cap_handler,
};

static const unsigned int
    payload_size_table[MOD_SCMI_POWER_CAPPING_REQ_COMMAND_COUNT] = {
        [MOD_SCMI_POWER_CAPPING_REQ_CAP_SET] =
            (unsigned int)sizeof(struct scmi_pcap_req_set_cap_a2p),
    };

static_assert(
    FWK_ARRAY_SIZE(handler_table) == FWK_ARRAY_SIZE(payload_size_table),
    "[SCMI] Power Capping Req protocol table sizes not "
    "consistent");

static struct scmi_power_capping_req_dev_ctx *get_dev_ctx_from_idx(
    unsigned int idx)
{
    if (idx >= mod_ctx.element_count) {
        FWK_LOG_ERR("[PCAP Req] Failed to get element ctx (%i).", idx);
        return NULL;
    }

    return &mod_ctx.dev_ctx_table[idx];
}

#ifdef BUILD_HAS_MOD_TIMER
static void set_cap_alarm_callback(uintptr_t param)
{
    struct scmi_power_capping_req_dev_ctx *dev_ctx;

    unsigned int id = (unsigned int)param;

    dev_ctx = get_dev_ctx_from_idx(id);
    fwk_assert(dev_ctx != NULL);

    /*
     * This flag is cleared to enable further requests as
     * it has not responded. Also a log message is raised to
     * note this.
     */
    dev_ctx->responded = true;
    FWK_LOG_ERR("[PCAP Req] Failed set cap response (%i).", id);
}
#endif

static int start_set_cap_alarm(fwk_id_t id)
{
#ifdef BUILD_HAS_MOD_TIMER
    struct scmi_power_capping_req_dev_ctx *dev_ctx =
        get_dev_ctx_from_idx(fwk_id_get_element_idx(id));
    fwk_assert(dev_ctx != NULL);

    if (mod_ctx.alarm_api == NULL) {
        return FWK_E_SUPPORT;
    }
    return mod_ctx.alarm_api->start(
        dev_ctx->config->alarm_id,
        dev_ctx->config->alarm_delay,
        MOD_TIMER_ALARM_TYPE_ONCE,
        set_cap_alarm_callback,
        (uintptr_t)fwk_id_get_element_idx(id));
#else
    return FWK_E_SUPPORT;
#endif
}

/*
 * SCMI module -> SCMI power capping requester module interface
 */
static int scmi_power_capping_req_get_scmi_protocol_id(
    fwk_id_t protocol_id,
    uint8_t *scmi_protocol_id)
{
    *scmi_protocol_id = (uint8_t)MOD_SCMI_PROTOCOL_ID_POWER_CAPPING;

    return FWK_SUCCESS;
}

/*
 * Power Capping Requester Response handlers
 */
static int scmi_power_capping_req_message_handler(
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

    alt_status = mod_ctx.scmi_api->response_message_handler(service_id);
    return (ret_status != FWK_SUCCESS) ? ret_status : alt_status;
}

static struct mod_scmi_to_protocol_api
    scmi_power_capping_req_scmi_to_protocol_api = {
        .get_scmi_protocol_id = scmi_power_capping_req_get_scmi_protocol_id,
        .message_handler = scmi_power_capping_req_message_handler,
    };

static bool try_get_element_idx_from_service(
    fwk_id_t service_id,
    uint32_t *element_idx)
{
    unsigned int i;

    for (i = 0; i < mod_ctx.element_count; i++) {
        if (fwk_id_is_equal(
                service_id, mod_ctx.dev_ctx_table[i].config->service_id)) {
            *element_idx = i;
            return true;
        }
    }
    return false;
}

/*
 * Return Power Capping Requester handler. This is the Set Cap response
 * handler.
 */
static int scmi_power_capping_req_set_cap_handler(
    fwk_id_t service_id,
    const void *payload,
    size_t payload_size)
{
    uint32_t element_idx;

    struct scmi_power_capping_req_dev_ctx *dev_ctx;

    if (try_get_element_idx_from_service(service_id, &element_idx)) {
        dev_ctx = get_dev_ctx_from_idx(element_idx);
        if ((dev_ctx != NULL) && dev_ctx->is_response_requested &&
            !dev_ctx->responded) {
            /* Mark it as responded now */
            dev_ctx->responded = true;

#ifdef BUILD_HAS_MOD_TIMER
            if (mod_ctx.alarm_api != NULL) {
                int status;
                /* Disable the timer as a response has been received */
                status = mod_ctx.alarm_api->stop(dev_ctx->config->alarm_id);

                if (!fwk_expect(status == FWK_SUCCESS)) {
                    FWK_LOG_ERR(
                        "[PCAP Req] Failed alarm stop (%i).", element_idx);
                }
            }
#endif
        }
    }
    return *((const int *)payload);
}

/*
 * API implementation
 */

static int set_power_cap(fwk_id_t id, uint32_t power_cap, uint32_t flags)
{
    uint32_t element_idx;
    int status;
    struct scmi_power_capping_req_dev_ctx *dev_ctx;

    /* Prepare the payload to send */
    const struct scmi_pcap_req_set_cap_a2p payload = {
        .flags = flags,
        .power_cap = (uint32_t)power_cap,
    };

    uint8_t scmi_protocol_id = (uint8_t)MOD_SCMI_PROTOCOL_ID_POWER_CAPPING;
    uint8_t scmi_message_id = (uint8_t)MOD_SCMI_POWER_CAPPING_REQ_CAP_SET;

    /* Check that the domain_id is valid element */
    if (!fwk_id_is_type(id, FWK_ID_TYPE_ELEMENT)) {
        return FWK_E_PARAM;
    }

    element_idx = fwk_id_get_element_idx(id);

    dev_ctx = get_dev_ctx_from_idx(element_idx);
    fwk_assert(dev_ctx != NULL);

    if (dev_ctx->responded == false) {
        return FWK_E_BUSY;
    }

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

    status = start_set_cap_alarm(id);
    /* Log error if timer incorrectly started */
    if (status != FWK_SUCCESS) {
        FWK_LOG_ERR("[PCAP Req] Failed alarm start (%i).", element_idx);
    }

    return FWK_SUCCESS;
}

/* API structure */
static const struct mod_scmi_power_capping_req_api power_capping_req_api = {
    .set_power_cap = set_power_cap,
};

/* Power management API implementation */

static int set_power_limit(fwk_id_t id, uint32_t power_limit)
{
    /* Disable Async responses */
    const uint32_t flags = SCMI_POWER_CAPPPING_REQ_SET_FLAGS(0, 0);

    return set_power_cap(id, power_limit, flags);
}

/* Power management API structure */
struct interface_power_management_api power_management_api = {
    .set_power_limit = set_power_limit,
};

/* Framework handler functions */
static int scmi_power_capping_req_init(
    fwk_id_t module_id,
    unsigned int element_count,
    const void *data)
{
    /* We definitely need elements in this module. */
    if (element_count == 0) {
        return FWK_E_SUPPORT;
    }

    mod_ctx.element_count = element_count;
    mod_ctx.dev_ctx_table =
        fwk_mm_calloc(element_count, sizeof(mod_ctx.dev_ctx_table[0]));
    mod_ctx.token = 0;

    return FWK_SUCCESS;
}

static int scmi_power_capping_req_elem_init(
    fwk_id_t element_id,
    unsigned int unused,
    const void *data)
{
    struct scmi_power_capping_req_dev_ctx *dev_ctx;

    if (fwk_id_get_element_idx(element_id) >= mod_ctx.element_count) {
        return FWK_E_PARAM;
    }

    dev_ctx = get_dev_ctx_from_idx(fwk_id_get_element_idx(element_id));
    fwk_assert(dev_ctx != NULL);

    if (data == NULL) {
        return FWK_E_PANIC;
    }

    dev_ctx->config = data;

#ifdef BUILD_HAS_MOD_TIMER
    if (fwk_optional_id_is_defined(dev_ctx->config->alarm_id)) {
        if (!fwk_module_is_valid_sub_element_id(dev_ctx->config->alarm_id) ||
            dev_ctx->config->alarm_delay == 0) {
            return FWK_E_SUPPORT;
        }
    }
#endif

    return FWK_SUCCESS;
}

static int scmi_power_capping_req_bind(fwk_id_t id, unsigned int round)
{
    int status = FWK_SUCCESS;

    if (round == 0) {
        if (fwk_id_is_type(id, FWK_ID_TYPE_MODULE)) {
            status = fwk_module_bind(
                FWK_ID_MODULE(FWK_MODULE_IDX_SCMI),
                FWK_ID_API(FWK_MODULE_IDX_SCMI, MOD_SCMI_API_IDX_PROTOCOL_REQ),
                &mod_ctx.scmi_api);
        }

        if (status != FWK_SUCCESS) {
            return status;
        }
    }

#ifdef BUILD_HAS_MOD_TIMER

    struct scmi_power_capping_req_dev_ctx *dev_ctx;
    unsigned int element_idx;
    element_idx = fwk_id_get_element_idx(id);

    if (element_idx >= mod_ctx.element_count) {
        return FWK_E_PARAM;
    }

    dev_ctx = get_dev_ctx_from_idx(element_idx);
    fwk_assert(dev_ctx != NULL);

    if (fwk_optional_id_is_defined(dev_ctx->config->alarm_id)) {
        status = fwk_module_bind(
            dev_ctx->config->alarm_id,
            MOD_TIMER_API_ID_ALARM,
            &mod_ctx.alarm_api);

        if (status != FWK_SUCCESS) {
            mod_ctx.alarm_api = NULL;
        }
    } else {
        mod_ctx.alarm_api = NULL;
    }
#endif

    return status;
}

static int scmi_power_capping_req_process_bind_request(
    fwk_id_t requester_id,
    fwk_id_t unused,
    fwk_id_t api_id,
    const void **api)
{
    enum mod_power_capping_req_api_idx api_idx;

    api_idx = (enum mod_power_capping_req_api_idx)fwk_id_get_api_idx(api_id);

    switch (api_idx) {
    case MOD_POW_CAP_REQ_API_IDX_SCMI_REQ:
        if (!fwk_id_is_equal(
                fwk_id_build_module_id(requester_id), fwk_module_id_scmi)) {
            return FWK_E_ACCESS;
        }

        *api = &scmi_power_capping_req_scmi_to_protocol_api;
        break;

    case MOD_POW_CAP_REQ_API_IDX_REQ:
        *api = &power_capping_req_api;
        break;

    case MOD_POW_CAP_REQ_API_IDX_LIMITER_POWER_API:
        *api = &power_management_api;
        break;

    default:
        return FWK_E_SUPPORT;
    }

    return FWK_SUCCESS;
}

/* Module descriptor */
const struct fwk_module module_scmi_power_capping_req = {
    .type = FWK_MODULE_TYPE_PROTOCOL,
    .api_count = (unsigned int)MOD_POW_CAP_REQ_API_IDX_COUNT,
    .init = scmi_power_capping_req_init,
    .element_init = scmi_power_capping_req_elem_init,
    .bind = scmi_power_capping_req_bind,
    .process_bind_request = scmi_power_capping_req_process_bind_request,
};
