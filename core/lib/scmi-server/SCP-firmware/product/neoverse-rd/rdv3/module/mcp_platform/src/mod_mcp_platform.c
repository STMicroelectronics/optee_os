/*
 * Arm SCP/MCP Software
 * Copyright (c) 2024-2025, Arm Limited and Contributors. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include "mcp_cfgd_scmi.h"
#include "mod_mcp_platform.h"
#include "nrd_scmi.h"

#include <mod_scmi.h>
#include <mod_scmi_sys_power.h>

#include <fwk_id.h>
#include <fwk_log.h>
#include <fwk_module.h>
#include <fwk_notification.h>
#include <fwk_status.h>

#include <fmw_cmsis.h>

#define MOD_NAME "[MCP_PLATFORM] "

#ifdef BUILD_HAS_SCMI_NOTIFICATIONS
/* Module context */
struct mcp_platform_ctx {
    /*! SCMI protocol API */
    const struct mod_scmi_from_protocol_req_api *scmi_protocol_req_api;
};

/* Module context data */
struct mcp_platform_ctx mcp_platform_ctx;

/* Notification for system power down */
static const fwk_id_t mod_scmi_sys_power_notification_system_power_down =
    FWK_ID_NOTIFICATION_INIT(
        FWK_MODULE_IDX_SCMI_SYS_POWER,
        MOD_SCMI_SYSTEM_POWER_NOTIFICATION_IDX_SYSTEM_POWER_DOWN);
#endif

/*
 * Framework handlers.
 */
static int mod_mcp_platform_init(
    fwk_id_t module_id,
    unsigned int element_count,
    const void *unused)
{
    return FWK_SUCCESS;
}

#ifdef BUILD_HAS_SCMI_NOTIFICATIONS
static int mod_mcp_platform_bind(fwk_id_t id, unsigned int round)
{
    int status = FWK_SUCCESS;

    if (round == 0) {
        /* Bind to SCMI module for SCP communication */
        status = fwk_module_bind(
            FWK_ID_MODULE(FWK_MODULE_IDX_SCMI),
            FWK_ID_API(FWK_MODULE_IDX_SCMI, MOD_SCMI_API_IDX_PROTOCOL_REQ),
            &mcp_platform_ctx.scmi_protocol_req_api);
        if (status != FWK_SUCCESS) {
            return status;
        }
    }

    return status;
}

/*
 * SCMI module -> MCP platform module interface
 */
static int platform_system_get_scmi_protocol_id(
    fwk_id_t protocol_id,
    uint8_t *scmi_protocol_id)
{
    *scmi_protocol_id = (uint8_t)MOD_SCMI_PROTOCOL_ID_SYS_POWER;

    return FWK_SUCCESS;
}

/*
 * Upon binding the mcp_platform module to the SCMI module, the SCMI module
 * will also bind back to the mcp_platform module, anticipating the presence of
 * .get_scmi_protocol_id() and .message_handler() APIs.
 *
 * In the current implementation of mcp_platform module, only sending SCMI
 * message is implemented, and the mcp_platform module is not intended to
 * receive any SCMI messages. Therefore, it is necessary to include a minimal
 * .message_handler() API to ensure the successful binding of the SCMI module.
 */
static int platform_system_scmi_message_handler(
    fwk_id_t protocol_id,
    fwk_id_t service_id,
    const uint32_t *payload,
    size_t payload_size,
    unsigned int message_id)
{
    return FWK_SUCCESS;
}

/* SCMI driver interface */
const struct mod_scmi_to_protocol_api platform_system_scmi_api = {
    .get_scmi_protocol_id = platform_system_get_scmi_protocol_id,
    .message_handler = platform_system_scmi_message_handler,
};

static int mod_mcp_platform_process_bind(
    fwk_id_t requester_id,
    fwk_id_t target_id,
    fwk_id_t api_id,
    const void **api)
{
    int status;
    enum mod_mcp_platform_api_idx api_id_type;

    api_id_type = (enum mod_mcp_platform_api_idx)fwk_id_get_api_idx(api_id);

    switch (api_id_type) {
    case MOD_MCP_PLATFORM_API_IDX_SCMI_POWER_DOWN:
        *api = &platform_system_scmi_api;
        status = FWK_SUCCESS;
        break;

    default:
        status = FWK_E_PARAM;
    }

    return status;
}
#endif

static int mod_mcp_platform_start(fwk_id_t id)
{
#ifdef BUILD_HAS_SCMI_NOTIFICATIONS
    /*
     * TODO: Need to sent the SCMI message only after SCP has completed the SCMI
     * initialization.
     */
    int status;

    fwk_id_t mcp_scmi_prot_id = FWK_ID_ELEMENT(
        FWK_MODULE_IDX_SCMI, MCP_CFGD_MOD_SCMI_EIDX_SCP_SCMI_SEND);
    struct mcp_cfgd_scmi_sys_power_state_notify_payload mcp_scmi_payload;

    mcp_scmi_payload.flags = 1;

    status = mcp_platform_ctx.scmi_protocol_req_api->scmi_send_message(
        MOD_SCMI_SYS_POWER_STATE_NOTIFY,
        MOD_SCMI_PROTOCOL_ID_SYS_POWER,
        0,
        mcp_scmi_prot_id,
        (void *)&mcp_scmi_payload,
        sizeof(mcp_scmi_payload),
        false);

    if (status != FWK_SUCCESS) {
        FWK_LOG_ERR(MOD_NAME
                    "Failed to subscribe SCMI power state change notofication");
        return status;
    }

#    ifdef BUILD_HAS_NOTIFICATION
    status = fwk_notification_subscribe(
        mod_scmi_sys_power_notification_system_power_down,
        FWK_ID_MODULE(FWK_MODULE_IDX_SCMI_SYS_POWER),
        id);
    if (status != FWK_SUCCESS) {
        FWK_LOG_ERR(MOD_NAME "Failed to subscribe to power down notification");
        return status;
    }
#    endif
#endif

    FWK_LOG_INFO(MOD_NAME "MCP RAM firmware initialized");
    return FWK_SUCCESS;
}

#ifdef BUILD_HAS_NOTIFICATION
static int mcp_platform_process_notification(
    const struct fwk_event *event,
    struct fwk_event *resp_event)
{
#    ifdef BUILD_HAS_SCMI_NOTIFICATIONS
    unsigned int *power_down;

    /* Notification handler for system wide power down. */
    if (fwk_id_is_equal(
            event->id, mod_scmi_sys_power_notification_system_power_down)) {
        power_down = (unsigned int *)event->params;

        if (*power_down == NRD_SCMI_SYSTEM_STATE_SHUTDOWN) {
            FWK_LOG_INFO(MOD_NAME "System shutting down!");
        } else if (*power_down == NRD_SCMI_SYSTEM_STATE_COLD_RESET) {
            FWK_LOG_INFO(MOD_NAME "System rebooting!");
        } else {
            FWK_LOG_ERR(MOD_NAME "Invalid power mode");
        }

        __WFI();
    } else {
        return FWK_E_PARAM;
    }
#    endif

    return FWK_SUCCESS;
}
#endif

const struct fwk_module module_mcp_platform = {
    .type = FWK_MODULE_TYPE_SERVICE,
    .init = mod_mcp_platform_init,
#ifdef BUILD_HAS_SCMI_NOTIFICATIONS
    .bind = mod_mcp_platform_bind,
    .process_bind_request = mod_mcp_platform_process_bind,
#endif
    .start = mod_mcp_platform_start,
#ifdef BUILD_HAS_NOTIFICATION
    .process_notification = mcp_platform_process_notification,
#endif
};

const struct fwk_module_config config_mcp_platform = { 0 };
