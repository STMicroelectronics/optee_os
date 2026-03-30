/*
 * Arm SCP/MCP Software
 * Copyright (c) 2025, Arm Limited and Contributors. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 * Description:
 *     Configuration data for module 'pfdi_monitor'.
 */

#include <mod_pfdi_monitor.h>

#include <fwk_id.h>
#include <fwk_module.h>
#include <fwk_module_idx.h>

#define SI0_SI0_TIMER_ALARM_ELEMENT_IDX  0
#define SI0_PFDI_MONITOR_ALARM_IDX_CORE0 0
#define SI0_PFDI_MONITOR_ALARM_IDX_CORE1 1

#define PFDI_OOR_PERIOD_US  5000000UL
#define PFDI_BOOT_PERIOD_US 50000000UL
#define PFDI_ONL_PERIOD_US  100000UL

enum si0_mod_pfdi_monitor_element_idx {
    SI0_MOD_PFDI_MONITOR_EIDX_CORE_0,
    SI0_MOD_PFDI_MONITOR_EIDX_CORE_1,
    SI0_MOD_PFDI_MONITOR_EIDX_COUNT,
};

#define PFDI_MONITOR_CORE(core) \
    { \
        .name = "Core " #core, \
        .data = &((const struct mod_pfdi_monitor_core_config){ \
            .alarm_id = FWK_ID_SUB_ELEMENT_INIT( \
                FWK_MODULE_IDX_TIMER, \
                SI0_SI0_TIMER_ALARM_ELEMENT_IDX, \
                SI0_PFDI_MONITOR_ALARM_IDX_CORE##core), \
            .oor_pfdi_period_us = PFDI_OOR_PERIOD_US, \
            .onl_pfdi_period_us = PFDI_ONL_PERIOD_US, \
            .boot_timeout_us = PFDI_BOOT_PERIOD_US, \
        }), \
    }

struct fwk_element element_table[] = {
    [SI0_MOD_PFDI_MONITOR_EIDX_CORE_0] = PFDI_MONITOR_CORE(0),
    [SI0_MOD_PFDI_MONITOR_EIDX_CORE_1] = PFDI_MONITOR_CORE(1),
    [SI0_MOD_PFDI_MONITOR_EIDX_COUNT] = { 0 },
};
