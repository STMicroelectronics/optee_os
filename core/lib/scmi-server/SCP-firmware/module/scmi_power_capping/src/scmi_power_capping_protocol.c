/*
 * Arm SCP/MCP Software
 * Copyright (c) 2023-2025, Arm Limited and Contributors. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 * Description:
 *      SCMI power capping and monitoring protocol completer.
 */
#include "internal/scmi_power_capping.h"
#include "internal/scmi_power_capping_core.h"
#include "internal/scmi_power_capping_fast_channels.h"
#include "internal/scmi_power_capping_protocol.h"

#include <mod_scmi.h>
#ifdef BUILD_HAS_MOD_RESOURCE_PERMS
#    include <mod_resource_perms.h>
#endif

#include <fwk_assert.h>
#include <fwk_core.h>
#include <fwk_event.h>
#include <fwk_id.h>
#include <fwk_mm.h>
#include <fwk_module.h>
#include <fwk_notification.h>
#include <fwk_string.h>

#include <stdbool.h>

#define MOD_SCMI_POWER_CAPPING_NOTIFICATION_COUNT 1

/*
 * SCMI Power Capping message handlers.
 */

static int scmi_power_capping_protocol_version_handler(
    fwk_id_t service_id,
    const uint32_t *payload);
static int scmi_power_capping_protocol_attributes_handler(
    fwk_id_t service_id,
    const uint32_t *payload);
static int scmi_power_capping_protocol_msg_attributes_handler(
    fwk_id_t service_id,
    const uint32_t *payload);
static int scmi_power_capping_domain_attributes_handler(
    fwk_id_t service_id,
    const uint32_t *payload);
static int scmi_power_capping_cap_get_handler(
    fwk_id_t service_id,
    const uint32_t *payload);
static int scmi_power_capping_cap_set_handler(
    fwk_id_t service_id,
    const uint32_t *payload);
static int scmi_power_capping_pai_get_handler(
    fwk_id_t service_id,
    const uint32_t *payload);
static int scmi_power_capping_pai_set_handler(
    fwk_id_t service_id,
    const uint32_t *payload);
static int scmi_power_capping_measurements_get_handler(
    fwk_id_t service_id,
    const uint32_t *payload);
#ifdef BUILD_HAS_SCMI_NOTIFICATIONS
static int scmi_power_capping_cap_notify_handler(
    fwk_id_t cap_service_id,
    const uint32_t *payload);
static int scmi_power_capping_measurements_notify_handler(
    fwk_id_t cap_service_id,
    const uint32_t *payload);
#endif
#ifdef BUILD_HAS_SCMI_POWER_CAPPING_FAST_CHANNELS_COMMANDS
static int scmi_power_capping_describe_fast_channel_handler(
    fwk_id_t cap_service_id,
    const uint32_t *payload);
#endif
/*
 * Internal variables.
 */

static struct {
    /* SCMI protocol module to SCMI module API */
    const struct mod_scmi_from_protocol_api *scmi_api;
#ifdef BUILD_HAS_SCMI_NOTIFICATIONS
    /* SCMI notification API */
    const struct mod_scmi_notification_api *scmi_notification_api;
#endif
#ifdef BUILD_HAS_MOD_RESOURCE_PERMS
    /* SCMI Resource Permissions API */
    const struct mod_res_permissions_api *res_perms_api;
#endif
} pcapping_protocol_ctx;

static handler_table_t handler_table[MOD_SCMI_POWER_CAPPING_COMMAND_COUNT] = {
    [MOD_SCMI_PROTOCOL_VERSION] = scmi_power_capping_protocol_version_handler,
    [MOD_SCMI_PROTOCOL_ATTRIBUTES] =
        scmi_power_capping_protocol_attributes_handler,
    [MOD_SCMI_PROTOCOL_MESSAGE_ATTRIBUTES] =
        scmi_power_capping_protocol_msg_attributes_handler,
    [MOD_SCMI_POWER_CAPPING_DOMAIN_ATTRIBUTES] =
        scmi_power_capping_domain_attributes_handler,
    [MOD_SCMI_POWER_CAPPING_CAP_GET] = scmi_power_capping_cap_get_handler,
    [MOD_SCMI_POWER_CAPPING_CAP_SET] = scmi_power_capping_cap_set_handler,
    [MOD_SCMI_POWER_CAPPING_PAI_GET] = scmi_power_capping_pai_get_handler,
    [MOD_SCMI_POWER_CAPPING_PAI_SET] = scmi_power_capping_pai_set_handler,
    [MOD_SCMI_POWER_CAPPING_MEASUREMENTS_GET] =
        scmi_power_capping_measurements_get_handler,
#ifdef BUILD_HAS_SCMI_NOTIFICATIONS
    [MOD_SCMI_POWER_CAPPING_CAP_NOTIFY] = scmi_power_capping_cap_notify_handler,
    [MOD_SCMI_POWER_CAPPING_MEASUREMENTS_NOTIFY] =
        scmi_power_capping_measurements_notify_handler,
#endif
#ifdef BUILD_HAS_SCMI_POWER_CAPPING_FAST_CHANNELS_COMMANDS
    [MOD_SCMI_POWER_CAPPING_DESCRIBE_FAST_CHANNEL] =
        scmi_power_capping_describe_fast_channel_handler,
#endif
};

static size_t payload_size_table[MOD_SCMI_POWER_CAPPING_COMMAND_COUNT] = {
    [MOD_SCMI_PROTOCOL_VERSION] = 0,
    [MOD_SCMI_PROTOCOL_ATTRIBUTES] = 0,
    [MOD_SCMI_PROTOCOL_MESSAGE_ATTRIBUTES] =
        sizeof(struct scmi_protocol_message_attributes_a2p),
    [MOD_SCMI_POWER_CAPPING_DOMAIN_ATTRIBUTES] =
        sizeof(struct scmi_power_capping_domain_attributes_a2p),
    [MOD_SCMI_POWER_CAPPING_CAP_GET] =
        sizeof(struct scmi_power_capping_cap_get_a2p),
    [MOD_SCMI_POWER_CAPPING_CAP_SET] =
        sizeof(struct scmi_power_capping_cap_set_a2p),
    [MOD_SCMI_POWER_CAPPING_PAI_GET] =
        sizeof(struct scmi_power_capping_pai_get_a2p),
    [MOD_SCMI_POWER_CAPPING_PAI_SET] =
        sizeof(struct scmi_power_capping_pai_set_a2p),
    [MOD_SCMI_POWER_CAPPING_MEASUREMENTS_GET] =
        sizeof(struct scmi_power_capping_measurements_get_a2p),
#ifdef BUILD_HAS_SCMI_NOTIFICATIONS
    [MOD_SCMI_POWER_CAPPING_CAP_NOTIFY] =
        sizeof(struct scmi_power_capping_cap_notify_a2p),
    [MOD_SCMI_POWER_CAPPING_MEASUREMENTS_NOTIFY] =
        sizeof(struct scmi_power_capping_measurements_notify_a2p),
#endif
#ifdef BUILD_HAS_SCMI_POWER_CAPPING_FAST_CHANNELS_COMMANDS
    [MOD_SCMI_POWER_CAPPING_DESCRIBE_FAST_CHANNEL] =
        sizeof(struct scmi_power_capping_describe_fc_a2p),
#endif
};

static_assert(
    FWK_ARRAY_SIZE(handler_table) == FWK_ARRAY_SIZE(payload_size_table),
    "[SCMI] Power capping protocol table sizes not consistent");

/*
 * Helper functions
 */

/*
 * Static helper function to process protocol, message and resource permissions.
 */
#ifdef BUILD_HAS_MOD_RESOURCE_PERMS
static int scmi_power_capping_permissions_handler(
    unsigned int message_id,
    fwk_id_t cap_service_id,
    const uint32_t *payload)
{
    unsigned int agent_id;
    enum mod_res_perms_permissions perms;
    int status;

    status =
        pcapping_protocol_ctx.scmi_api->get_agent_id(cap_service_id, &agent_id);

    if (status != FWK_SUCCESS) {
        return FWK_E_ACCESS;
    }

    perms = pcapping_protocol_ctx.res_perms_api->agent_has_resource_permission(
        agent_id, MOD_SCMI_PROTOCOL_ID_POWER_CAPPING, message_id, *payload);

    if (perms == MOD_RES_PERMS_ACCESS_DENIED) {
        status = FWK_E_ACCESS;
    }

    return status;
}
#endif

/*
 * Static helper function to check if a message is implemented.
 */
static bool scmi_power_capping_is_msg_implemented(
    fwk_id_t service_id,
    uint32_t message_id)
{
    return (
        (message_id < FWK_ARRAY_SIZE(handler_table)) &&
        (handler_table[message_id] != NULL));
}

/*
 * Static helper function to map return status into scmi error code
 */

enum scmi_error map_scmi_error(int status)
{
    enum scmi_error scmi_return_error;

    switch (status) {
    case FWK_E_RANGE:
        scmi_return_error = SCMI_OUT_OF_RANGE;
        break;
    case FWK_E_SUPPORT:
        scmi_return_error = SCMI_NOT_SUPPORTED;
        break;
    case FWK_E_BUSY:
        scmi_return_error = SCMI_BUSY;
        break;
    default:
        scmi_return_error = SCMI_GENERIC_ERROR;
        break;
    }

    return scmi_return_error;
}

/*
 * Static helper function to set disabled pai values
 */
static void pcapping_protocol_set_disabled_pai_values(
    uint32_t *pai_step,
    uint32_t *min_pai,
    uint32_t *max_pai)
{
    *pai_step = 0u;
    *min_pai = UNSUPPORTED_CONFIG_PAI_VALUE;
    *max_pai = UNSUPPORTED_CONFIG_PAI_VALUE;
}

/*
 * Static helper function to populate domain attributes return values.
 */
static inline void scmi_power_capping_populate_domain_attributes(
    struct scmi_power_capping_domain_attributes_p2a *return_values,
    unsigned int domain_idx,
    const struct mod_scmi_power_capping_domain_config *config)
{
    bool cap_support;

    (void)pcapping_core_get_cap_support(domain_idx, &cap_support);

    return_values->attributes = SCMI_POWER_CAPPING_DOMAIN_ATTRIBUTES(
        cap_support, config->pai_config_support, config->power_cap_unit);

#ifdef BUILD_HAS_SCMI_POWER_CAPPING_FAST_CHANNELS_COMMANDS
    return_values->attributes |= SCMI_POWER_CAPPING_DOMAIN_FCH_SUPPORT(
        pcapping_fast_channel_get_domain_support(service_id, domain_idx));
#endif
#ifdef BUILD_HAS_SCMI_NOTIFICATIONS
    return_values->attributes |=
        SCMI_POWER_CAPPING_DOMAIN_CAP_PAI_CHANGE_NOTIF_SUPPORT(
            config->cap_pai_change_notification_support);
    return_values->attributes |=
        SCMI_POWER_CAPPING_DOMAIN_MEASUREMENTS_NOTIF_SUPPORT(
            config->power_measurements_change_notification_support);
#endif
}

/*
 * Static helper to handle error scmi responses
 */
static int scmi_power_capping_respond_error(
    fwk_id_t service_id,
    enum scmi_error scmi_error)
{
    int return_status = (int)scmi_error;
    return pcapping_protocol_ctx.scmi_api->respond(
        service_id, &return_status, sizeof(return_status));
}

/*
 * Power capping protocol implementation
 */
static int scmi_power_capping_protocol_version_handler(
    fwk_id_t service_id,
    const uint32_t *payload)
{
    struct scmi_protocol_version_p2a return_values = {
        .status = (int32_t)SCMI_SUCCESS,
        .version = SCMI_PROTOCOL_VERSION_POWER_CAPPING,
    };

    return pcapping_protocol_ctx.scmi_api->respond(
        service_id, &return_values, sizeof(return_values));
}

static int scmi_power_capping_protocol_attributes_handler(
    fwk_id_t service_id,
    const uint32_t *payload)
{
    unsigned int domain_count = pcapping_core_get_domain_count();
    struct scmi_protocol_attributes_p2a return_values = {
        .status = (int32_t)SCMI_SUCCESS,
        .attributes = domain_count,
    };

    return pcapping_protocol_ctx.scmi_api->respond(
        service_id, &return_values, sizeof(return_values));
}

static int scmi_power_capping_protocol_msg_attributes_handler(
    fwk_id_t service_id,
    const uint32_t *payload)
{
    const struct scmi_protocol_message_attributes_a2p *parameters;
    struct scmi_protocol_message_attributes_p2a return_values;
#ifdef BUILD_HAS_MOD_RESOURCE_PERMS
    int status;
#endif
    parameters = (const struct scmi_protocol_message_attributes_a2p *)payload;

    if (!scmi_power_capping_is_msg_implemented(
            service_id, parameters->message_id)) {
        return scmi_power_capping_respond_error(service_id, SCMI_NOT_FOUND);
    }

#ifdef BUILD_HAS_MOD_RESOURCE_PERMS
    status = scmi_power_capping_permissions_handler(
        parameters->message_id, service_id, payload);
    if (status != FWK_SUCCESS) {
        return scmi_power_capping_respond_error(service_id, SCMI_NOT_FOUND);
    }
#endif

#ifdef BUILD_HAS_SCMI_POWER_CAPPING_FAST_CHANNELS_COMMANDS
    return_values.attributes =
        pcapping_fast_channel_get_msg_support(parameters->message_id) ?
        SCMI_POWER_CAPPING_FCH_AVAIL :
        SCMI_POWER_CAPPING_FCH_NOT_AVAIL;
#else
    return_values.attributes = SCMI_POWER_CAPPING_FCH_NOT_AVAIL;
#endif

    return_values.status = SCMI_SUCCESS;
    return pcapping_protocol_ctx.scmi_api->respond(
        service_id, &return_values, sizeof(return_values));
}

static int scmi_power_capping_domain_attributes_handler(
    fwk_id_t service_id,
    const uint32_t *payload)
{
    const struct scmi_power_capping_domain_attributes_a2p *parameters;
    struct scmi_power_capping_domain_attributes_p2a return_values = { 0 };
    const struct mod_scmi_power_capping_domain_config *config;
    int status;

    parameters =
        (const struct scmi_power_capping_domain_attributes_a2p *)payload;

    status = pcapping_core_get_config(parameters->domain_id, &config);

    if (status != FWK_SUCCESS) {
        return status;
    }

    scmi_power_capping_populate_domain_attributes(
        &return_values, parameters->domain_id, config);

    if (config->pai_config_support == false) {
        pcapping_protocol_set_disabled_pai_values(
            &return_values.pai_step,
            &return_values.min_pai,
            &return_values.max_pai);
    } else {
        status = pcapping_core_get_pai_info(
            parameters->domain_id,
            &return_values.min_pai,
            &return_values.max_pai,
            &return_values.pai_step);
        if (status != FWK_SUCCESS) {
            return scmi_power_capping_respond_error(
                service_id, SCMI_GENERIC_ERROR);
        }
    }

    return_values.min_power_cap = config->min_power_cap;
    return_values.max_power_cap = config->max_power_cap;
    return_values.power_cap_step = config->power_cap_step;
    return_values.max_sustainable_power = config->max_sustainable_power;
    return_values.parent_id = config->parent_idx;

    fwk_str_strncpy(
        (char *)return_values.name,
        fwk_module_get_element_name(FWK_ID_ELEMENT(
            FWK_MODULE_IDX_SCMI_POWER_CAPPING, parameters->domain_id)),
        sizeof(return_values.name) - 1);

    return_values.status = SCMI_SUCCESS;

    return pcapping_protocol_ctx.scmi_api->respond(
        service_id, &return_values, sizeof(return_values));
}

static int scmi_power_capping_cap_get_handler(
    fwk_id_t service_id,
    const uint32_t *payload)
{
    const struct scmi_power_capping_cap_get_a2p *parameters;
    struct scmi_power_capping_cap_get_p2a return_values;
    int status;

    parameters = (const struct scmi_power_capping_cap_get_a2p *)payload;

    status =
        pcapping_core_get_cap(parameters->domain_id, &return_values.power_cap);

    if (status != FWK_SUCCESS) {
        return scmi_power_capping_respond_error(service_id, SCMI_GENERIC_ERROR);
    }

    return_values.status = SCMI_SUCCESS;

    return pcapping_protocol_ctx.scmi_api->respond(
        service_id, &return_values, sizeof(return_values));
}

static int scmi_power_capping_cap_set_handler(
    fwk_id_t service_id,
    const uint32_t *payload)
{
    const struct scmi_power_capping_cap_set_a2p *parameters;
    struct scmi_power_capping_cap_set_p2a return_values;
    bool async_flag;
    bool delayed_res_flag;
    int status;

    parameters = (const struct scmi_power_capping_cap_set_a2p *)payload;

    if ((parameters->flags & SCMI_POWER_CAPPING_INVALID_MASK) != 0) {
        return scmi_power_capping_respond_error(
            service_id, SCMI_INVALID_PARAMETERS);
    }

    async_flag = (parameters->flags & SCMI_POWER_CAPPING_ASYNC_FLAG_MASK) ==
        SCMI_POWER_CAPPING_ASYNC_FLAG_MASK;

    if (async_flag) {
        delayed_res_flag =
            !((parameters->flags & SCMI_POWER_CAPPING_IGN_DEL_RES_FLAG_MASK) ==
              SCMI_POWER_CAPPING_IGN_DEL_RES_FLAG_MASK);

        if (delayed_res_flag) {
            return scmi_power_capping_respond_error(
                service_id, SCMI_NOT_SUPPORTED);
        }
    }

    status = pcapping_core_set_cap(
        service_id, parameters->domain_id, async_flag, parameters->power_cap);

    if ((status != FWK_PENDING) && (status != FWK_SUCCESS)) {
        return scmi_power_capping_respond_error(
            service_id, map_scmi_error(status));
    }
    if ((status == FWK_PENDING) && (!async_flag)) {
        return FWK_SUCCESS;
    }

    return_values.status = SCMI_SUCCESS;

    return pcapping_protocol_ctx.scmi_api->respond(
        service_id, &return_values, sizeof(return_values));
}

static int scmi_power_capping_pai_get_handler(
    fwk_id_t service_id,
    const uint32_t *payload)
{
    const struct scmi_power_capping_pai_get_a2p *parameters;
    struct scmi_power_capping_pai_get_p2a return_values;
    uint32_t pai;
    int status;

    parameters = (const struct scmi_power_capping_pai_get_a2p *)payload;

    status = pcapping_core_get_pai(parameters->domain_id, &pai);

    if (status != FWK_SUCCESS) {
        return scmi_power_capping_respond_error(
            service_id, map_scmi_error(status));
    }

    return_values.pai = pai;
    return_values.status = SCMI_SUCCESS;

    return pcapping_protocol_ctx.scmi_api->respond(
        service_id, &return_values, sizeof(return_values));
}

static int scmi_power_capping_pai_set_handler(
    fwk_id_t service_id,
    const uint32_t *payload)
{
    const struct scmi_power_capping_pai_set_a2p *parameters;
    struct scmi_power_capping_pai_set_p2a return_values;
    int status;

    parameters = (const struct scmi_power_capping_pai_set_a2p *)payload;

    if (parameters->flags != SCMI_POWER_CAPPING_PAI_RESERVED_FLAG) {
        return scmi_power_capping_respond_error(
            service_id, SCMI_INVALID_PARAMETERS);
    }

    status = pcapping_core_set_pai(
        service_id, parameters->domain_id, parameters->pai);

    if (status != FWK_SUCCESS) {
        return scmi_power_capping_respond_error(
            service_id, map_scmi_error(status));
    }

    return_values.status = SCMI_SUCCESS;

    return pcapping_protocol_ctx.scmi_api->respond(
        service_id, &return_values, sizeof(return_values));
}

static int scmi_power_capping_measurements_get_handler(
    fwk_id_t service_id,
    const uint32_t *payload)
{
    const struct scmi_power_capping_measurements_get_a2p *parameters;
    struct scmi_power_capping_measurements_get_p2a return_values;
    int status;

    parameters =
        (const struct scmi_power_capping_measurements_get_a2p *)payload;

    status =
        pcapping_core_get_power(parameters->domain_id, &return_values.power);

    if (status != FWK_SUCCESS) {
        return scmi_power_capping_respond_error(
            service_id, map_scmi_error(status));
    }

    status = pcapping_core_get_pai(parameters->domain_id, &return_values.pai);

    if (status != FWK_SUCCESS) {
        return scmi_power_capping_respond_error(
            service_id, map_scmi_error(status));
    }

    return_values.status = SCMI_SUCCESS;

    return pcapping_protocol_ctx.scmi_api->respond(
        service_id, &return_values, sizeof(return_values));
}

#ifdef BUILD_HAS_SCMI_NOTIFICATIONS
static int scmi_power_capping_cap_notify_handler(
    fwk_id_t cap_service_id,
    const uint32_t *payload)
{
    struct scmi_power_capping_cap_notify_a2p *parameters;
    struct scmi_power_capping_cap_notify_p2a return_values;
    unsigned int agent_id;
    int status;

    parameters = (struct scmi_power_capping_cap_notify_a2p *)payload;

    if (parameters->notify_enable) {
        status = pcapping_protocol_ctx.scmi_notification_api
                     ->scmi_notification_add_subscriber(
                         MOD_SCMI_PROTOCOL_ID_POWER_CAPPING,
                         parameters->domain_id,
                         MOD_SCMI_POWER_CAPPING_CAP_NOTIFY,
                         cap_service_id);
    } else {
        status = pcapping_protocol_ctx.scmi_api->get_agent_id(
            cap_service_id, &agent_id);

        if (status != FWK_SUCCESS) {
            return scmi_power_capping_respond_error(
                cap_service_id, SCMI_GENERIC_ERROR);
        }

        status = pcapping_protocol_ctx.scmi_notification_api
                     ->scmi_notification_remove_subscriber(
                         MOD_SCMI_PROTOCOL_ID_POWER_CAPPING,
                         agent_id,
                         parameters->domain_id,
                         MOD_SCMI_POWER_CAPPING_CAP_NOTIFY);
    }

    if (status != FWK_SUCCESS) {
        return scmi_power_capping_respond_error(
            cap_service_id, SCMI_GENERIC_ERROR);
    }

    return_values.status = SCMI_SUCCESS;

    return pcapping_protocol_ctx.scmi_api->respond(
        cap_service_id, &return_values, sizeof(return_values));
}

static int scmi_power_capping_measurements_notify_handler(
    fwk_id_t cap_service_id,
    const uint32_t *payload)
{
    struct scmi_power_capping_measurements_notify_a2p *parameters;
    struct scmi_power_capping_measurements_notify_p2a return_values;
    unsigned int agent_id;
    int status;

    parameters = (struct scmi_power_capping_measurements_notify_a2p *)payload;

    if (parameters->notify_enable) {
        status = pcapping_protocol_ctx.scmi_notification_api
                     ->scmi_notification_add_subscriber(
                         MOD_SCMI_PROTOCOL_ID_POWER_CAPPING,
                         parameters->domain_id,
                         MOD_SCMI_POWER_CAPPING_MEASUREMENTS_NOTIFY,
                         cap_service_id);

        if (status != FWK_SUCCESS) {
            return scmi_power_capping_respond_error(
                cap_service_id, SCMI_GENERIC_ERROR);
        }

        status = pcapping_core_set_power_thresholds(
            parameters->domain_id,
            parameters->threshold_low,
            parameters->threshold_high);

    } else {
        status = pcapping_protocol_ctx.scmi_api->get_agent_id(
            cap_service_id, &agent_id);

        if (status != FWK_SUCCESS) {
            return scmi_power_capping_respond_error(
                cap_service_id, SCMI_GENERIC_ERROR);
        }

        status = pcapping_protocol_ctx.scmi_notification_api
                     ->scmi_notification_remove_subscriber(
                         MOD_SCMI_PROTOCOL_ID_POWER_CAPPING,
                         agent_id,
                         parameters->domain_id,
                         MOD_SCMI_POWER_CAPPING_MEASUREMENTS_NOTIFY);
    }

    if (status != FWK_SUCCESS) {
        return scmi_power_capping_respond_error(
            cap_service_id, SCMI_GENERIC_ERROR);
    }

    return_values.status = SCMI_SUCCESS;

    return pcapping_protocol_ctx.scmi_api->respond(
        cap_service_id, &return_values, sizeof(return_values));
}
#endif

#ifdef BUILD_HAS_SCMI_POWER_CAPPING_FAST_CHANNELS_COMMANDS
static int scmi_power_capping_describe_fast_channel_handler(
    fwk_id_t cap_service_id,
    const uint32_t *payload)
{
    const struct scmi_power_capping_describe_fc_a2p *parameters;
    struct scmi_power_capping_describe_fc_p2a return_values = { 0 };
    struct pcapping_fast_channel_info info;
    unsigned int domain_idx;
    uint32_t message_id;
    int status;

    parameters = (const struct scmi_power_capping_describe_fc_a2p *)payload;
    domain_idx = parameters->domain_id;
    message_id = parameters->message_id;

    status = pcapping_fast_channel_get_info(domain_idx, message_id, &info);

    if (status != FWK_SUCCESS) {
        return_values.status =
            status == FWK_E_RANGE ? SCMI_NOT_FOUND : SCMI_NOT_SUPPORTED;
        return pcapping_protocol_ctx.scmi_api->respond(
            cap_service_id, &return_values, sizeof(return_values.status));
    }

    return_values.chan_addr_high = (uint32_t)((uint64_t)info.fch_address >> 32);
    return_values.chan_addr_low = (uint32_t)info.fch_address;

    return_values.chan_size = info.fch_channel_size;

    return_values.rate_limit = info.fch_rate_limit;

    return_values.attributes = info.fch_attributes;

    return_values.status = SCMI_SUCCESS;

    return pcapping_protocol_ctx.scmi_api->respond(
        cap_service_id, &return_values, sizeof(return_values));
}
#endif

/*
 * SCMI module -> SCMI power capping module interface
 */
static int scmi_power_capping_get_scmi_protocol_id(
    fwk_id_t protocol_id,
    uint8_t *scmi_protocol_id)
{
    fwk_assert(scmi_protocol_id != NULL);
    *scmi_protocol_id = (uint8_t)MOD_SCMI_PROTOCOL_ID_POWER_CAPPING;

    return FWK_SUCCESS;
}

static int scmi_power_capping_message_handler(
    fwk_id_t protocol_id,
    fwk_id_t service_id,
    const uint32_t *payload,
    size_t payload_size,
    unsigned int message_id)
{
    int validation_result;

    validation_result = pcapping_protocol_ctx.scmi_api->scmi_message_validation(
        MOD_SCMI_PROTOCOL_ID_POWER_CAPPING,
        service_id,
        payload,
        payload_size,
        message_id,
        payload_size_table,
        (unsigned int)MOD_SCMI_POWER_CAPPING_COMMAND_COUNT,
        handler_table);

    if (validation_result == SCMI_SUCCESS) {
        return handler_table[message_id](service_id, payload);
    } else {
        return pcapping_protocol_ctx.scmi_api->respond(
            service_id, &validation_result, sizeof(validation_result));
    }
}

static struct mod_scmi_to_protocol_api
    scmi_power_capping_mod_scmi_to_protocol_api = {
        .get_scmi_protocol_id = scmi_power_capping_get_scmi_protocol_id,
        .message_handler = scmi_power_capping_message_handler
    };

int pcapping_protocol_bind(void)
{
    int status;

    status = fwk_module_bind(
        FWK_ID_MODULE(FWK_MODULE_IDX_SCMI),
        FWK_ID_API(FWK_MODULE_IDX_SCMI, MOD_SCMI_API_IDX_PROTOCOL),
        &(pcapping_protocol_ctx.scmi_api));

#ifdef BUILD_HAS_MOD_RESOURCE_PERMS
    if (status != FWK_SUCCESS) {
        return status;
    }

    status = fwk_module_bind(
        FWK_ID_MODULE(FWK_MODULE_IDX_RESOURCE_PERMS),
        FWK_ID_API(FWK_MODULE_IDX_RESOURCE_PERMS, MOD_RES_PERM_RESOURCE_PERMS),
        &(pcapping_protocol_ctx.res_perms_api));
#endif

#ifdef BUILD_HAS_SCMI_NOTIFICATIONS
    if (status != FWK_SUCCESS) {
        return status;
    }

    status = fwk_module_bind(
        FWK_ID_MODULE(FWK_MODULE_IDX_SCMI),
        FWK_ID_API(FWK_MODULE_IDX_SCMI, MOD_SCMI_API_IDX_NOTIFICATION),
        &(pcapping_protocol_ctx.scmi_notification_api));
#endif
    return status;
}

#ifdef BUILD_HAS_SCMI_NOTIFICATIONS
int pcapping_protocol_start(void)
{
    int status;
    unsigned int agent_count;

    status = pcapping_protocol_ctx.scmi_api->get_agent_count(&agent_count);
    if (status != FWK_SUCCESS) {
        return status;
    }

    fwk_assert(agent_count != 0u);

    status =
        pcapping_protocol_ctx.scmi_notification_api->scmi_notification_init(
            MOD_SCMI_PROTOCOL_ID_POWER_CAPPING,
            agent_count,
            pcapping_core_get_domain_count(),
            MOD_SCMI_POWER_CAPPING_NOTIFICATION_COUNT);

    return status;
}
#endif

int pcapping_protocol_process_bind_request(fwk_id_t api_id, const void **api)
{
    if (fwk_id_is_equal(
            api_id,
            FWK_ID_API(
                FWK_MODULE_IDX_SCMI_POWER_CAPPING,
                MOD_SCMI_POWER_CAPPING_API_IDX_REQUEST))) {
        *api = &scmi_power_capping_mod_scmi_to_protocol_api;
        return FWK_SUCCESS;
    }

    return FWK_E_SUPPORT;
}

int pcapping_protocol_process_cap_pai_notify_event(
    const struct fwk_event *event)
{
    int status = FWK_SUCCESS;
    struct pcapping_core_cap_pai_event_parameters *event_params =
        (struct pcapping_core_cap_pai_event_parameters *)event->params;

    if (!pcapping_core_is_cap_request_async(event_params->domain_idx)) {
        struct scmi_power_capping_cap_set_p2a cap_set_return_values;
        cap_set_return_values.status = (int)SCMI_SUCCESS;

        status = pcapping_protocol_ctx.scmi_api->respond(
            event_params->service_id,
            &cap_set_return_values,
            sizeof(cap_set_return_values));
    }

#ifdef BUILD_HAS_SCMI_NOTIFICATIONS
    if (status != FWK_SUCCESS) {
        return status;
    }

    unsigned int cap_pai_notif_agent_id;

    if (!fwk_id_is_equal(event_params->service_id, FWK_ID_NONE)) {
        status = pcapping_protocol_ctx.scmi_api->get_agent_id(
            event_params->service_id, &cap_pai_notif_agent_id);

        if (status != FWK_SUCCESS) {
            return status;
        }
    } else {
        cap_pai_notif_agent_id = SCMI_POWER_CAPPING_AGENT_ID_PLATFORM;
    }

    struct scmi_power_capping_cap_changed_p2a scmi_notification_payload;

    scmi_notification_payload.agent_id = cap_pai_notif_agent_id;
    scmi_notification_payload.domain_id = event_params->domain_idx;
    scmi_notification_payload.cap = event_params->cap;
    scmi_notification_payload.pai = event_params->pai;

    return pcapping_protocol_ctx.scmi_notification_api
        ->scmi_notification_notify(
            MOD_SCMI_PROTOCOL_ID_POWER_CAPPING,
            MOD_SCMI_POWER_CAPPING_CAP_NOTIFY,
            event_params->domain_idx,
            SCMI_POWER_CAPPING_CAP_CHANGED,
            &scmi_notification_payload,
            sizeof(struct scmi_power_capping_cap_changed_p2a));
#else
    return status;
#endif
}

#ifdef BUILD_HAS_SCMI_NOTIFICATIONS
int pcapping_protocol_process_measurements_notify_event(
    const struct fwk_event *event)
{
    struct scmi_power_capping_measurements_changed_p2a payload = { 0 };

    struct pcapping_core_pwr_meas_event_parameters *event_params =
        (struct pcapping_core_pwr_meas_event_parameters *)event->params;

    payload.agent_id = SCMI_POWER_CAPPING_AGENT_ID_PLATFORM;
    payload.domain_id = event_params->domain_idx;
    payload.power = event_params->power;

    return pcapping_protocol_ctx.scmi_notification_api
        ->scmi_notification_notify(
            MOD_SCMI_PROTOCOL_ID_POWER_CAPPING,
            MOD_SCMI_POWER_CAPPING_MEASUREMENTS_NOTIFY,
            event_params->domain_idx,
            SCMI_POWER_CAPPING_MEASUREMENTS_CHANGED,
            &payload,
            sizeof(struct scmi_power_capping_measurements_changed_p2a));
}
#endif
