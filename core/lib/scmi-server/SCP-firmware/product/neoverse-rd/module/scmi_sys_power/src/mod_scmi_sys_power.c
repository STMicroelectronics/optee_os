/*
 * Arm SCP/MCP Software
 * Copyright (c) 2024-2025, Arm Limited and Contributors. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <internal/scmi_sys_power.h>

#include <mod_scmi.h>
#include <mod_scmi_sys_power.h>

#include <fwk_assert.h>
#include <fwk_attributes.h>
#include <fwk_id.h>
#include <fwk_log.h>
#include <fwk_macros.h>
#include <fwk_mm.h>
#include <fwk_module.h>
#include <fwk_module_idx.h>
#include <fwk_notification.h>
#include <fwk_status.h>
#include <fwk_string.h>

#include <stddef.h>

#define INVALID_AGENT_ID UINT32_MAX

struct mod_scmi_sys_power_ctx {
    const struct mod_scmi_from_protocol_api *scmi_api;
};

static int scmi_sys_power_version_handler(
    fwk_id_t service_id,
    const uint32_t *payload);

static int scmi_sys_power_attributes_handler(
    fwk_id_t service_id,
    const uint32_t *payload);

static int scmi_sys_power_msg_attributes_handler(
    fwk_id_t service_id,
    const uint32_t *payload);

#ifdef BUILD_HAS_SCMI_NOTIFICATIONS
static int scmi_sys_power_state_notifier_handler(
    fwk_id_t service_id,
    const uint32_t *payload);

/* Notification for system power down */
static const fwk_id_t mod_scmi_sys_power_notification_system_power_down =
    FWK_ID_NOTIFICATION_INIT(
        FWK_MODULE_IDX_SCMI_SYS_POWER,
        MOD_SCMI_SYSTEM_POWER_NOTIFICATION_IDX_SYSTEM_POWER_DOWN);
#endif

/*
 * Internal variables
 */
static struct mod_scmi_sys_power_ctx scmi_sys_power_ctx;

/*
 * Add support for the three basic SCMI message:
 * 1. protocol version
 * 2. protocol attribute
 * 3. protocol message attributes
 * These three messages are common for all SCMI protocols.
 */
static int (*const message_handler_table[MOD_SCMI_SYS_POWER_COMMAND_COUNT])(
    fwk_id_t,
    const uint32_t *) = {
    [MOD_SCMI_PROTOCOL_VERSION] = scmi_sys_power_version_handler,
    [MOD_SCMI_PROTOCOL_ATTRIBUTES] = scmi_sys_power_attributes_handler,
    [MOD_SCMI_PROTOCOL_MESSAGE_ATTRIBUTES] =
        scmi_sys_power_msg_attributes_handler,
};

/*
 * Input payload size of the basic SCMI messages as specified in SCMI v3.2
 */
static const unsigned int
    payload_size_table[MOD_SCMI_SYS_POWER_COMMAND_COUNT] = {
        /* No input payload */
        [MOD_SCMI_PROTOCOL_VERSION] = 0,
        /* No input payload */
        [MOD_SCMI_PROTOCOL_ATTRIBUTES] = 0,
        /* payload specified in struct scmi_protocol_message_attributes_a2p */
        [MOD_SCMI_PROTOCOL_MESSAGE_ATTRIBUTES] =
            (unsigned int)sizeof(struct scmi_protocol_message_attributes_a2p),
    };

#ifdef BUILD_HAS_SCMI_NOTIFICATIONS
/*
 * Support for SCMI system power protocol notification
 */
static int (
        *const notification_handler_table[SCMI_SYS_POWER_NOTIFICATION_COUNT])(
    fwk_id_t,
    const uint32_t *) = {
    [SCMI_SYS_POWER_STATE_SET_NOTIFY] = scmi_sys_power_state_notifier_handler,
};

static const unsigned int
    payload_size_table_notification[SCMI_SYS_POWER_NOTIFICATION_COUNT] = {
        [SCMI_SYS_POWER_STATE_SET_NOTIFY] =
            sizeof(struct scmi_sys_power_state_notifier),
    };
#endif

/*
 * PROTOCOL_VERSION
 */
static int scmi_sys_power_version_handler(
    fwk_id_t service_id,
    const uint32_t *payload)
{
    struct scmi_protocol_version_p2a return_values = {
        .status = (int32_t)SCMI_SUCCESS,
        .version = SCMI_PROTOCOL_VERSION_SYS_POWER,
    };

    return scmi_sys_power_ctx.scmi_api->respond(
        service_id, &return_values, sizeof(return_values));
}

/*
 * PROTOCOL_ATTRIBUTES
 */
static int scmi_sys_power_attributes_handler(
    fwk_id_t service_id,
    const uint32_t *payload)
{
    struct scmi_protocol_attributes_p2a return_values = {
        .status = (int32_t)SCMI_SUCCESS,
        .attributes = 0,
    };

    return scmi_sys_power_ctx.scmi_api->respond(
        service_id, &return_values, sizeof(return_values));
}

/*
 * PROTOCOL_MESSAGE_ATTRIBUTES
 */
static int scmi_sys_power_msg_attributes_handler(
    fwk_id_t service_id,
    const uint32_t *payload)
{
    struct scmi_protocol_message_attributes_p2a return_values;

    /*
     * The module currently support only SCMI notification, not SCMI message,
     * hence return SCMI_NOT_SUPPORTED for all message attribute query.
     */
    return_values.status = (int32_t)SCMI_NOT_SUPPORTED;

    return scmi_sys_power_ctx.scmi_api->respond(
        service_id, &return_values, sizeof(return_values.status));
}

/*
 * SCMI module -> SCMI system power module interface
 */
static int scmi_sys_power_get_scmi_protocol_id(
    fwk_id_t protocol_id,
    uint8_t *scmi_protocol_id)
{
    *scmi_protocol_id = (uint8_t)MOD_SCMI_PROTOCOL_ID_SYS_POWER;

    return FWK_SUCCESS;
}

/*
 * Message handler for generic SCMI messages
 */
static int scmi_sys_power_message_handler(
    fwk_id_t protocol_id,
    fwk_id_t service_id,
    const uint32_t *payload,
    size_t payload_size,
    unsigned int message_id)
{
    int32_t return_value;

    static_assert(
        FWK_ARRAY_SIZE(message_handler_table) ==
            FWK_ARRAY_SIZE(payload_size_table),
        "[SCMI] System power protocol table sizes not consistent");

    fwk_assert(payload != NULL);

    if (message_id >= FWK_ARRAY_SIZE(message_handler_table)) {
        return_value = (int32_t)SCMI_NOT_FOUND;
        goto error;
    }

    if (payload_size != payload_size_table[message_id]) {
        /* Incorrect payload size or message is not supported */
        return_value = (int32_t)SCMI_PROTOCOL_ERROR;
        goto error;
    }

    return message_handler_table[message_id](service_id, payload);

error:
    return scmi_sys_power_ctx.scmi_api->respond(
        service_id, &return_value, sizeof(return_value));
}

#ifdef BUILD_HAS_SCMI_NOTIFICATIONS
/*
 * SYSTEM_POWER_STATE_NOTIFIER
 */
static int scmi_sys_power_state_notifier_handler(
    fwk_id_t service_id,
    const uint32_t *payload)
{
    int status = FWK_SUCCESS;
    int32_t return_value;
    const struct scmi_sys_power_state_notifier *parameters;
#    ifdef BUILD_HAS_NOTIFICATION
    unsigned int count;
    struct fwk_event notification_event = {
        .id = mod_scmi_sys_power_notification_system_power_down,
        .response_requested = false,
        .source_id = FWK_ID_MODULE_INIT(FWK_MODULE_IDX_SCMI_SYS_POWER),
    };
#    endif

    parameters = (const struct scmi_sys_power_state_notifier *)payload;

    if (parameters->flags & (uint32_t)(~STATE_NOTIFY_FLAGS_MASK)) {
        return_value = (int32_t)SCMI_INVALID_PARAMETERS;
        goto exit;
    }

#    ifdef BUILD_HAS_NOTIFICATION
    fwk_str_memcpy(
        notification_event.params,
        &parameters->system_state,
        sizeof(parameters->system_state));
    status = fwk_notification_notify(&notification_event, &count);
    if (status != FWK_SUCCESS) {
        FWK_LOG_ERR(
            "[SCMI_SYSTEM_POWER] failed to notify power state transition: %s",
            fwk_status_str(status));
        return SCMI_DENIED;
    }
#    endif

exit:
    status = scmi_sys_power_ctx.scmi_api->respond(
        service_id, &return_value, sizeof(return_value));

    return status;
}

/*
 * Handler for SCMI system power notifications
 */
static int scmi_sys_power_notification_handler(
    fwk_id_t protocol_id,
    fwk_id_t service_id,
    const uint32_t *payload,
    size_t payload_size,
    unsigned int notification_id)
{
    int32_t return_value;

    static_assert(
        FWK_ARRAY_SIZE(notification_handler_table) ==
            FWK_ARRAY_SIZE(payload_size_table_notification),
        "[SCMI] System power protocol notification table sizes not consistent");

    fwk_assert(payload != NULL);

    if (notification_id >= FWK_ARRAY_SIZE(notification_handler_table)) {
        return_value = (int32_t)SCMI_NOT_FOUND;
        goto error;
    }

    if (payload_size != payload_size_table_notification[notification_id]) {
        /* Incorrect payload size or message is not supported */
        return_value = (int32_t)SCMI_PROTOCOL_ERROR;
        goto error;
    }

    return notification_handler_table[notification_id](service_id, payload);

error:
    return scmi_sys_power_ctx.scmi_api->respond(
        service_id, &return_value, sizeof(return_value));
}
#endif

static struct mod_scmi_to_protocol_api scmi_sys_power_mod_scmi_to_protocol = {
    .get_scmi_protocol_id = scmi_sys_power_get_scmi_protocol_id,
    .message_handler = scmi_sys_power_message_handler,
#ifdef BUILD_HAS_SCMI_NOTIFICATIONS
    .notification_handler = scmi_sys_power_notification_handler,
#endif
};

/*
 * Framework handlers
 */
static int scmi_sys_power_init(
    fwk_id_t module_id,
    unsigned int element_count,
    const void *data)
{
    return FWK_SUCCESS;
}

static int scmi_sys_power_bind(fwk_id_t id, unsigned int round)
{
    int status;

    if (round != 0) {
        return FWK_SUCCESS;
    }

    /* Bind to SCMI module */
    status = fwk_module_bind(
        FWK_ID_MODULE(FWK_MODULE_IDX_SCMI),
        FWK_ID_API(FWK_MODULE_IDX_SCMI, MOD_SCMI_API_IDX_PROTOCOL),
        &scmi_sys_power_ctx.scmi_api);
    if (status != FWK_SUCCESS) {
        return status;
    }

    return FWK_SUCCESS;
}

static int scmi_sys_power_process_bind_request(
    fwk_id_t source_id,
    fwk_id_t _target_id,
    fwk_id_t api_id,
    const void **api)
{
    if (!fwk_id_is_equal(source_id, FWK_ID_MODULE(FWK_MODULE_IDX_SCMI))) {
        return FWK_E_ACCESS;
    }

    *api = &scmi_sys_power_mod_scmi_to_protocol;

    return FWK_SUCCESS;
}

const struct fwk_module module_scmi_sys_power = {
    .api_count = 1,
    .notification_count = MOD_SCMI_SYSTEM_POWER_NOTIFICATION_COUNT,
    .type = FWK_MODULE_TYPE_PROTOCOL,
    .init = scmi_sys_power_init,
    .bind = scmi_sys_power_bind,
    .process_bind_request = scmi_sys_power_process_bind_request,
};

const struct fwk_module_config config_scmi_sys_power = { 0 };
