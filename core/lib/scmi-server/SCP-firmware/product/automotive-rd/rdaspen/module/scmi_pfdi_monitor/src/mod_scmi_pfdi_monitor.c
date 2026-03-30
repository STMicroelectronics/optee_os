/*
 * Arm SCP/MCP Software
 * Copyright (c) 2025, Arm Limited and Contributors. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <mod_pfdi_monitor.h>
#include <mod_scmi.h>
#include <mod_scmi_pfdi_monitor.h>

#include <fwk_core.h>
#include <fwk_id.h>
#include <fwk_mm.h>
#include <fwk_module.h>

#define MOD_NAME "[SCMI_PFDI_MONITOR] "

/*! SCMI PFDI Monitor protocol ID */
#define SCMI_PFDI_MONITOR_PROTOCOL_ID UINT32_C(0x90)
/*! SCMI PFDI Monitor version */
#define SCMI_PFDI_MONITOR_PROTOCOL_VERSION UINT32_C(0x20000)
/*! PFDI Monitor Parameter Count */
#define SCMI_PFDI_MONITOR_OOR_STATUS_PARAMETER_COUNT 1
#define SCMI_PFDI_MONITOR_ONL_STATUS_PARAMETER_COUNT 1

/*!
 * \brief SCMI PFDI monitor response.
 */
struct scmi_pfdi_monitor_refresh_pfdi_monitor_resp {
    /*! SCMI status. */
    int status;
};

/*!
 * \brief Identifiers of the SCMI PFDI monitor commands.
 */
enum scmi_pfdi_monitor_command_id {
    /*! Out-Of-Reset PFDI Status */
    SCMI_PFDI_MONITOR_OOR_STATUS = 0x3,
    /*! Online PFDI Status */
    SCMI_PFDI_MONITOR_ONL_STATUS = 0x4,
};

static unsigned int payload_size_table[] = {
    [MOD_SCMI_PROTOCOL_VERSION] = 0,
    [MOD_SCMI_PROTOCOL_ATTRIBUTES] = 0,
    [MOD_SCMI_PROTOCOL_MESSAGE_ATTRIBUTES] =
        sizeof(struct scmi_protocol_message_attributes_a2p),
    [SCMI_PFDI_MONITOR_OOR_STATUS] =
        SCMI_PFDI_MONITOR_OOR_STATUS_PARAMETER_COUNT * sizeof(uint32_t),
    [SCMI_PFDI_MONITOR_ONL_STATUS] =
        SCMI_PFDI_MONITOR_ONL_STATUS_PARAMETER_COUNT * sizeof(uint32_t),
};

/*!
 * \brief Core context.
 */
struct scmi_pfdi_monitor_core_ctx {
    /* Module configuration */
    const struct mod_scmi_pfdi_monitor_core_config *cfg;
};

/*!
 * \brief Module context.
 */
struct scmi_pfdi_monitor_ctx {
    /* Core context table */
    struct scmi_pfdi_monitor_core_ctx *core_ctx_table;
    /* Number of elements */
    uint32_t core_count;
    /* SCMI protocol API */
    const struct mod_scmi_from_protocol_api *scmi_api;
    /* PFDI monitor API */
    const struct mod_pfdi_monitor_api *pfdi_monitor_api;
};

static struct scmi_pfdi_monitor_ctx ctx;

static int scmi_pfdi_monitor_protocol_version_handler(
    fwk_id_t service_id,
    const uint32_t *payload);
static int scmi_pfdi_monitor_protocol_attributes_handler(
    fwk_id_t service_id,
    const uint32_t *payload);
static int scmi_pfdi_monitor_protocol_msg_attributes_handler(
    fwk_id_t service_id,
    const uint32_t *payload);
static int scmi_pfdi_monitor_protocol_oor_status(
    fwk_id_t service_id,
    const uint32_t *payload);
static int scmi_pfdi_monitor_protocol_onl_status(
    fwk_id_t service_id,
    const uint32_t *payload);

static int (*handler_table[])(fwk_id_t, const uint32_t *) = {
    [MOD_SCMI_PROTOCOL_VERSION] = scmi_pfdi_monitor_protocol_version_handler,
    [MOD_SCMI_PROTOCOL_ATTRIBUTES] =
        scmi_pfdi_monitor_protocol_attributes_handler,
    [MOD_SCMI_PROTOCOL_MESSAGE_ATTRIBUTES] =
        scmi_pfdi_monitor_protocol_msg_attributes_handler,
    [SCMI_PFDI_MONITOR_OOR_STATUS] = scmi_pfdi_monitor_protocol_oor_status,
    [SCMI_PFDI_MONITOR_ONL_STATUS] = scmi_pfdi_monitor_protocol_onl_status,
};

static int scmi_pfdi_monitor_protocol_version_handler(
    fwk_id_t service_id,
    const uint32_t *payload)
{
    struct scmi_protocol_version_p2a return_values = {
        .status = SCMI_SUCCESS,
        .version = SCMI_PFDI_MONITOR_PROTOCOL_VERSION,
    };

    ctx.scmi_api->respond(service_id, &return_values, sizeof(return_values));

    return FWK_SUCCESS;
}

static int scmi_pfdi_monitor_protocol_attributes_handler(
    fwk_id_t service_id,
    const uint32_t *payload)
{
    struct scmi_protocol_attributes_p2a return_values = {
        .status = SCMI_SUCCESS,
    };

    ctx.scmi_api->respond(service_id, &return_values, sizeof(return_values));

    return FWK_SUCCESS;
}

static int scmi_pfdi_monitor_protocol_msg_attributes_handler(
    fwk_id_t service_id,
    const uint32_t *payload)
{
    const struct scmi_protocol_message_attributes_a2p *parameters;
    struct scmi_protocol_message_attributes_p2a return_values;

    parameters = (const struct scmi_protocol_message_attributes_a2p *)payload;

    if ((parameters->message_id < FWK_ARRAY_SIZE(handler_table)) &&
        (handler_table[parameters->message_id] != NULL)) {
        return_values = (struct scmi_protocol_message_attributes_p2a){
            .status = SCMI_SUCCESS,
            /*! All commands have an attributes value of 0 */
            .attributes = 0,
        };
    } else {
        return_values.status = SCMI_NOT_FOUND;
    }

    ctx.scmi_api->respond(
        service_id,
        &return_values,
        (return_values.status == SCMI_SUCCESS) ? sizeof(return_values) :
                                                 sizeof(return_values.status));

    return FWK_SUCCESS;
}

static int scmi_pfdi_monitor_protocol_oor_status(
    fwk_id_t service_id,
    const uint32_t *payload)
{
    int scmi_status = SCMI_SUCCESS, pfdi_monitor_status;
    const struct mod_scmi_pfdi_monitor_core_config *element = NULL;
    uint32_t status = payload[0];

    /*! Find the corresponding SCMI PFDI monitor element */
    for (uint32_t idx = 0; idx < ctx.core_count; idx++) {
        const struct mod_scmi_pfdi_monitor_core_config *current_element =
            ctx.core_ctx_table[idx].cfg;
        if (fwk_id_is_equal(current_element->scmi_service_id, service_id)) {
            element = current_element;
            break;
        }
    }

    if (element == NULL) {
        scmi_status = SCMI_INVALID_PARAMETERS;
        goto exit;
    }

    pfdi_monitor_status =
        ctx.pfdi_monitor_api->oor_status(element->pfdi_monitor_id, status);
    if (pfdi_monitor_status == FWK_SUCCESS) {
        scmi_status = SCMI_SUCCESS;
    } else if (pfdi_monitor_status == FWK_E_PARAM) {
        scmi_status = SCMI_INVALID_PARAMETERS;
    } else {
        scmi_status = SCMI_GENERIC_ERROR;
    }

exit:
    struct scmi_pfdi_monitor_refresh_pfdi_monitor_resp resp = {
        .status = scmi_status,
    };

    ctx.scmi_api->respond(service_id, &resp, sizeof(resp));

    return FWK_SUCCESS;
}

static int scmi_pfdi_monitor_protocol_onl_status(
    fwk_id_t service_id,
    const uint32_t *payload)
{
    int scmi_status = SCMI_SUCCESS, pfdi_monitor_status;
    const struct mod_scmi_pfdi_monitor_core_config *element = NULL;
    uint32_t status = payload[0];

    /*! Find the corresponding SCMI PFDI monitor element */
    for (uint32_t idx = 0; idx < ctx.core_count; idx++) {
        const struct mod_scmi_pfdi_monitor_core_config *current_element =
            ctx.core_ctx_table[idx].cfg;
        if (fwk_id_is_equal(current_element->scmi_service_id, service_id)) {
            element = current_element;
            break;
        }
    }

    if (element == NULL) {
        scmi_status = SCMI_INVALID_PARAMETERS;
        goto exit;
    }

    pfdi_monitor_status =
        ctx.pfdi_monitor_api->onl_status(element->pfdi_monitor_id, status);
    if (pfdi_monitor_status == FWK_SUCCESS) {
        scmi_status = SCMI_SUCCESS;
    } else if (pfdi_monitor_status == FWK_E_PARAM) {
        scmi_status = SCMI_INVALID_PARAMETERS;
    } else {
        scmi_status = SCMI_GENERIC_ERROR;
    }

exit:
    struct scmi_pfdi_monitor_refresh_pfdi_monitor_resp resp = {
        .status = scmi_status,
    };

    ctx.scmi_api->respond(service_id, &resp, sizeof(resp));

    return FWK_SUCCESS;
}

/**
 * @brief SCMI module to SCMI PFDI monitor module interface.
 */
static int scmi_pfdi_monitor_get_scmi_protocol_id(
    fwk_id_t protocol_id,
    uint8_t *scmi_protocol_id)
{
    if (scmi_protocol_id == NULL) {
        return FWK_E_PARAM;
    }

    *scmi_protocol_id = SCMI_PFDI_MONITOR_PROTOCOL_ID;

    return FWK_SUCCESS;
}

static int scmi_pfdi_monitor_message_handler(
    fwk_id_t protocol_id,
    fwk_id_t service_id,
    const uint32_t *payload,
    size_t payload_size,
    unsigned int message_id)
{
    int32_t return_value;

    static_assert(
        FWK_ARRAY_SIZE(handler_table) == FWK_ARRAY_SIZE(payload_size_table),
        MOD_NAME "SCMI table sizes not consistent");
    fwk_assert(payload != NULL);

    if (message_id >= FWK_ARRAY_SIZE(handler_table)) {
        return_value = SCMI_NOT_FOUND;
        goto error;
    }

    if (payload_size != payload_size_table[message_id]) {
        return_value = SCMI_PROTOCOL_ERROR;
        goto error;
    }

    return handler_table[message_id](service_id, payload);

error:
    ctx.scmi_api->respond(service_id, &return_value, sizeof(return_value));
    return FWK_SUCCESS;
}

static struct mod_scmi_to_protocol_api
    scmi_pfdi_monitor_mod_scmi_to_protocol_api = {
        .get_scmi_protocol_id = scmi_pfdi_monitor_get_scmi_protocol_id,
        .message_handler = scmi_pfdi_monitor_message_handler
    };

/**
 * @brief Framework interface.
 */
static int scmi_pfdi_monitor_init(
    fwk_id_t module_id,
    unsigned int element_count,
    const void *unused)
{
    if (element_count == 0) {
        return FWK_E_PARAM;
    }

    ctx.core_ctx_table =
        fwk_mm_calloc(element_count, sizeof(struct scmi_pfdi_monitor_core_ctx));

    ctx.core_count = element_count;

    return FWK_SUCCESS;
}

static int scmi_pfdi_monitor_element_init(
    fwk_id_t id,
    unsigned int sub_element_count,
    const void *data)
{
    const struct mod_scmi_pfdi_monitor_core_config *core_cfg;
    unsigned int element_idx = fwk_id_get_element_idx(id);

    if (element_idx >= ctx.core_count) {
        return FWK_E_PARAM;
    }

    core_cfg = (struct mod_scmi_pfdi_monitor_core_config *)data;

    if (core_cfg == NULL) {
        return FWK_E_DATA;
    }

    ctx.core_ctx_table[element_idx].cfg = core_cfg;

    return FWK_SUCCESS;
}

static int scmi_pfdi_monitor_bind(fwk_id_t id, unsigned int round)
{
    int status;

    if (round != 0) {
        return FWK_SUCCESS;
    }

    if (!fwk_id_is_type(id, FWK_ID_TYPE_MODULE)) {
        return FWK_SUCCESS;
    }

    status = fwk_module_bind(
        FWK_ID_MODULE(FWK_MODULE_IDX_SCMI),
        FWK_ID_API(FWK_MODULE_IDX_SCMI, MOD_SCMI_API_IDX_PROTOCOL),
        &ctx.scmi_api);

    if (status != FWK_SUCCESS) {
        return status;
    }

    return fwk_module_bind(
        FWK_ID_MODULE(FWK_MODULE_IDX_PFDI_MONITOR),
        FWK_ID_API(
            FWK_MODULE_IDX_PFDI_MONITOR, MOD_PFDI_MONITOR_API_IDX_PFDI_MONITOR),
        &ctx.pfdi_monitor_api);
}

static int scmi_pfdi_monitor_process_bind_request(
    fwk_id_t source_id,
    fwk_id_t target_id,
    fwk_id_t api_id,
    const void **api)
{
    /*! Only accept binding requests from the SCMI module. */
    if (!fwk_id_is_equal(source_id, FWK_ID_MODULE(FWK_MODULE_IDX_SCMI))) {
        return FWK_E_ACCESS;
    }

    if (api == NULL) {
        return FWK_E_PARAM;
    }

    *api = &scmi_pfdi_monitor_mod_scmi_to_protocol_api;

    return FWK_SUCCESS;
}

const struct fwk_module module_scmi_pfdi_monitor = {
    .api_count = 1U,
    .type = FWK_MODULE_TYPE_PROTOCOL,
    .init = scmi_pfdi_monitor_init,
    .element_init = scmi_pfdi_monitor_element_init,
    .bind = scmi_pfdi_monitor_bind,
    .process_bind_request = scmi_pfdi_monitor_process_bind_request,
};
