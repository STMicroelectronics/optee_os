/*
 * Arm SCP/MCP Software
 * Copyright (c) 2025, Arm Limited and Contributors. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 * Description:
 *     SCP Platform Support - Power Management
 */

#include "si0_cfgd_scmi.h"

#include <internal/si0_platform.h>

#include <mod_scmi.h>
#include <mod_scmi_std.h>
#include <mod_system_power.h>

#include <fwk_arch.h>
#include <fwk_assert.h>
#include <fwk_module.h>
#include <fwk_module_idx.h>
#include <fwk_status.h>

/*! SCMI protocol API */
static const struct mod_scmi_from_protocol_req_api *scmi_protocol_req_api;

/*
 * SCMI module -> SI0 platform module interface
 */
static int platform_system_get_scmi_protocol_id(
    fwk_id_t protocol_id,
    uint8_t *scmi_protocol_id)
{
    if (scmi_protocol_id == NULL) {
        return FWK_E_PARAM;
    }

    *scmi_protocol_id = (uint8_t)MOD_SCMI_PROTOCOL_ID_SYS_POWER;

    return FWK_SUCCESS;
}

/*
 * Upon binding the si0_platform module to the SCMI module, the SCMI module
 * will also bind back to the si0_platform module, anticipating the presence of
 * .get_scmi_protocol_id() and .message_handler() APIs.
 *
 * In the current implementation of si0_platform module, only sending SCMI
 * message is implemented, and the si0_platform module is not intended to
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

const void *get_platform_scmi_power_down_api(void)
{
    return &platform_system_scmi_api;
}

static int platform_shutdown(enum mod_pd_system_shutdown system_shutdown)
{
    while (true) {
#ifndef BUILD_TESTS
        asm("wfi");
#endif
    }

    return FWK_E_DEVICE;
}

/* Module 'system_power' driver interface */
const struct mod_system_power_driver_api platform_system_pwr_drv_api = {
    .system_shutdown = platform_shutdown,
};

const void *get_platform_system_power_driver_api(void)
{
    return &platform_system_pwr_drv_api;
}

int platform_power_mgmt_bind(void)
{
    int status;

    /* Bind to SCMI module */
    status = fwk_module_bind(
        FWK_ID_MODULE(FWK_MODULE_IDX_SCMI),
        FWK_ID_API(FWK_MODULE_IDX_SCMI, MOD_SCMI_API_IDX_PROTOCOL_REQ),
        &scmi_protocol_req_api);

    return status;
}
