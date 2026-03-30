/*
 * Arm SCP/MCP Software
 * Copyright (c) 2024, Arm Limited and Contributors. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 * Description:
 *     SCP platform sub-system initialization support.
 */

#include "core_manager.h"
#include "platform_core.h"

#include <internal/scp_platform.h>

#include <mod_scp_platform.h>
#include <mod_system_info.h>
#include <mod_transport.h>

#include <fwk_assert.h>
#include <fwk_id.h>
#include <fwk_log.h>
#include <fwk_module.h>
#include <fwk_module_idx.h>
#include <fwk_notification.h>
#include <fwk_status.h>

/* Module context */
struct scp_platform_ctx {
    /* Module config data */
    const struct mod_scp_platform_config *config;

    /* System Information HAL API pointer */
    struct mod_system_info_get_info_api *system_info_api;
};

/* Module context data */
struct scp_platform_ctx scp_platform_ctx;

static void platform_update_gpt_size(void)
{
    uint8_t core_idx;
    uint8_t core_count;

    core_count = platform_get_core_count();

    /* Configure the L0 GPT size for all AP cores */
    for (core_idx = 0; core_idx < core_count; core_idx++) {
        SCP_CU_AP_PERIPH_CORE_MANAGER_PTR(core_idx)->PE_STATIC_CONFIG &=
            ~(CORE_MANAGER_PE_STATIC_CONFIG_L0GPTSZ_MASK
              << CORE_MANAGER_PE_STATIC_CONFIG_L0GPTSZ_SHIFT);
        SCP_CU_AP_PERIPH_CORE_MANAGER_PTR(core_idx)->PE_STATIC_CONFIG |=
            (CORE_MANAGER_PE_STATIC_CONFIG_L0GPTSZ_16GB
             << CORE_MANAGER_PE_STATIC_CONFIG_L0GPTSZ_SHIFT);
    }
}

/*
 * Framework handlers.
 */
static int scp_platform_init(
    fwk_id_t module_id,
    unsigned int unused,
    const void *data)
{
    const struct mod_scp_platform_config *config;

    config = (struct mod_scp_platform_config *)data;

    if (!fwk_id_type_is_valid(config->timer_id) ||
        !fwk_id_type_is_valid(config->transport_id)) {
        return FWK_E_DATA;
    }

    /* Save the config data in the module context */
    scp_platform_ctx.config = config;

    return FWK_SUCCESS;
}

static int scp_platform_bind(fwk_id_t id, unsigned int round)
{
    int status;
    fwk_id_t mod_system_info_api_id_get_info;

    if (round > 0) {
        return FWK_SUCCESS;
    }

    /* Bind to modules required to handshake with RSE */
    status = platform_rse_bind(scp_platform_ctx.config);
    if (status != FWK_SUCCESS) {
        return status;
    }

    /* Bind to modules required for power management */
    status = platform_power_mgmt_bind();
    if (status != FWK_SUCCESS) {
        return status;
    }

    mod_system_info_api_id_get_info =
        FWK_ID_API(FWK_MODULE_IDX_SYSTEM_INFO, MOD_SYSTEM_INFO_GET_API_IDX);

    /* Bind to System Info HAL API */
    return fwk_module_bind(
        fwk_module_id_system_info,
        mod_system_info_api_id_get_info,
        &scp_platform_ctx.system_info_api);
}

static int scp_platform_process_bind_request(
    fwk_id_t requester_id,
    fwk_id_t target_id,
    fwk_id_t api_id,
    const void **api)
{
    int status;
    enum mod_scp_platform_api_idx api_idx;

    api_idx = (enum mod_scp_platform_api_idx)fwk_id_get_api_idx(api_id);

    switch (api_idx) {
    case MOD_SCP_PLATFORM_API_IDX_SYSTEM_POWER_DRIVER:
        *api = get_platform_system_power_driver_api();
        status = FWK_SUCCESS;
        break;

    case MOD_SCP_PLATFORM_API_IDX_TRANSPORT_SIGNAL:
        *api = get_platform_transport_signal_api();
        status = FWK_SUCCESS;
        break;

    default:
        status = FWK_E_PARAM;
    }

    return status;
}

static int scp_platform_start(fwk_id_t id)
{
    int status;
    const struct mod_system_info *system_info;
    struct fwk_event event = { 0 };
    unsigned int event_count;

    /* Notify RSE that SYSTOP is powered up and wait for RSE doorbell */
    status = notify_rse_and_wait_for_response();
    if (status != FWK_SUCCESS) {
        FWK_LOG_ERR(MOD_NAME "Error! SCP-RSE handshake failed");
        return FWK_E_PANIC;
    }

    /* SCP subsystem initialization completion notification */
    event.id = mod_scp_platform_notification_subsys_init;
    event.source_id = id;

    /*
     * RSE has now setup GPC bypass in the system control block. Notify other
     * modules that are waiting to access memory regions outside the SCP
     * subsystem (which otherwise would have generated a fault).
     */
    status = fwk_notification_notify(&event, &event_count);
    if (status != FWK_SUCCESS) {
        FWK_LOG_ERR(MOD_NAME "Error! Subsystem init notification failed");
        return FWK_E_PANIC;
    }

    /* Configure LCP0 UART access and release all LCPs */
    platform_setup_lcp();

    /* Update L0GPTSZ for all cores */
    platform_update_gpt_size();

    /* Determine the chip information */
    status = scp_platform_ctx.system_info_api->get_system_info(&system_info);
    if (status != FWK_SUCCESS) {
        FWK_LOG_ERR(MOD_NAME "Error! Failed to obtain system info");
        return FWK_E_PANIC;
    }

    if (system_info->chip_id != 0) {
        /* Nothing to be done for secondary chips */
        return FWK_SUCCESS;
    }

    FWK_LOG_INFO(MOD_NAME "Initializing the primary core...");

    status = init_ap_core(0);
    if (status != FWK_SUCCESS) {
        FWK_LOG_ERR(MOD_NAME "Error! Failed to initialize primary core");
        fwk_trap();
    }

    return FWK_SUCCESS;
}

const struct fwk_module module_scp_platform = {
    .type = FWK_MODULE_TYPE_DRIVER,
    .api_count = MOD_SCP_PLATFORM_API_COUNT,
    .notification_count = MOD_SCP_PLATFORM_NOTIFICATION_COUNT,
    .init = scp_platform_init,
    .bind = scp_platform_bind,
    .process_bind_request = scp_platform_process_bind_request,
    .start = scp_platform_start,
};
