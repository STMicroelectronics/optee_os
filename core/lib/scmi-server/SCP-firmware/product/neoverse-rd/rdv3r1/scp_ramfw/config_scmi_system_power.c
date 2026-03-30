/*
 * Arm SCP/MCP Software
 * Copyright (c) 2024, Arm Limited and Contributors. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 * Description:
 *     Configuration data for module 'scmi_system_power'.
 */

#include "scp_cfgd_timer.h"

#include <mod_scmi_system_power.h>
#include <mod_system_power.h>

#include <fwk_module.h>

const struct fwk_module_config config_scmi_system_power = {
    .data = &((struct mod_scmi_system_power_config){
        .system_view = MOD_SCMI_SYSTEM_VIEW_FULL,
        .system_suspend_state = MOD_SYSTEM_POWER_POWER_STATE_SLEEP0,
#ifdef BUILD_HAS_SCMI_NOTIFICATIONS
        .alarm_id = FWK_ID_SUB_ELEMENT_INIT(
            FWK_MODULE_IDX_TIMER,
            0,
            SCP_CFGD_SCMI_NOTIFICATION_ALARM_IDX),
        .graceful_timeout = 1000000, /* us */
#endif
    }),
};
