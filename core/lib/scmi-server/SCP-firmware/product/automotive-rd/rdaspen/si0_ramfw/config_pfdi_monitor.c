/*
 * Arm SCP/MCP Software
 * Copyright (c) 2025, Arm Limited and Contributors. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 * Description:
 *     Configuration data for module 'pfdi_monitor'.
 */

#include "si0_cfgd_pfdi_monitor.h"
#include "si0_cfgd_timer.h"
#include "si0_cfgd_transport.h"

#include <mod_pfdi_monitor.h>

#include <fwk_id.h>
#include <fwk_module.h>
#include <fwk_module_idx.h>

#define OOR_PFDI_PERIOD_US 5000000UL /* 5 seconds */
#define BOOT_TIMEOUT_US    50000000UL /* 50 seconds */

#define PFDI_MONITOR_AP_CORE(cluster, core) \
    { \
        .name = "AP cluster " #cluster " core " #core, \
        .data = &((const struct mod_pfdi_monitor_core_config){ \
            .alarm_id = FWK_ID_SUB_ELEMENT_INIT( \
                FWK_MODULE_IDX_TIMER, \
                SI0_SI0_TIMER_ALARM_ELEMENT_IDX, \
                SI0_CFGD_PFDI_MONITOR_ALARM_IDX_AP_CLUSTER##cluster##_CORE##core), \
            .oor_pfdi_period_us = OOR_PFDI_PERIOD_US, \
            .onl_pfdi_period_us = PFDI_ONLINE_TIMEOUT_US, \
            .boot_timeout_us = BOOT_TIMEOUT_US, \
        }), \
    }

static struct fwk_element element_table[] = {
    [SI0_CFGD_MOD_PFDI_MONITOR_EIDX_AP_CLUSTER_0_CORE_0] =
        PFDI_MONITOR_AP_CORE(0, 0),
    [SI0_CFGD_MOD_PFDI_MONITOR_EIDX_AP_CLUSTER_0_CORE_1] =
        PFDI_MONITOR_AP_CORE(0, 1),
    [SI0_CFGD_MOD_PFDI_MONITOR_EIDX_AP_CLUSTER_0_CORE_2] =
        PFDI_MONITOR_AP_CORE(0, 2),
    [SI0_CFGD_MOD_PFDI_MONITOR_EIDX_AP_CLUSTER_0_CORE_3] =
        PFDI_MONITOR_AP_CORE(0, 3),
    [SI0_CFGD_MOD_PFDI_MONITOR_EIDX_AP_CLUSTER_1_CORE_0] =
        PFDI_MONITOR_AP_CORE(1, 0),
    [SI0_CFGD_MOD_PFDI_MONITOR_EIDX_AP_CLUSTER_1_CORE_1] =
        PFDI_MONITOR_AP_CORE(1, 1),
    [SI0_CFGD_MOD_PFDI_MONITOR_EIDX_AP_CLUSTER_1_CORE_2] =
        PFDI_MONITOR_AP_CORE(1, 2),
    [SI0_CFGD_MOD_PFDI_MONITOR_EIDX_AP_CLUSTER_1_CORE_3] =
        PFDI_MONITOR_AP_CORE(1, 3),
    [SI0_CFGD_MOD_PFDI_MONITOR_EIDX_AP_CLUSTER_2_CORE_0] =
        PFDI_MONITOR_AP_CORE(2, 0),
    [SI0_CFGD_MOD_PFDI_MONITOR_EIDX_AP_CLUSTER_2_CORE_1] =
        PFDI_MONITOR_AP_CORE(2, 1),
    [SI0_CFGD_MOD_PFDI_MONITOR_EIDX_AP_CLUSTER_2_CORE_2] =
        PFDI_MONITOR_AP_CORE(2, 2),
    [SI0_CFGD_MOD_PFDI_MONITOR_EIDX_AP_CLUSTER_2_CORE_3] =
        PFDI_MONITOR_AP_CORE(2, 3),
    [SI0_CFGD_MOD_PFDI_MONITOR_EIDX_AP_CLUSTER_3_CORE_0] =
        PFDI_MONITOR_AP_CORE(3, 0),
    [SI0_CFGD_MOD_PFDI_MONITOR_EIDX_AP_CLUSTER_3_CORE_1] =
        PFDI_MONITOR_AP_CORE(3, 1),
    [SI0_CFGD_MOD_PFDI_MONITOR_EIDX_AP_CLUSTER_3_CORE_2] =
        PFDI_MONITOR_AP_CORE(3, 2),
    [SI0_CFGD_MOD_PFDI_MONITOR_EIDX_AP_CLUSTER_3_CORE_3] =
        PFDI_MONITOR_AP_CORE(3, 3),
    [SI0_CFGD_MOD_PFDI_MONITOR_EIDX_COUNT] = { 0 },
};

static const struct fwk_element *get_pfdi_monitor_element_table(fwk_id_t unused)
{
    element_table[PC_CONFIGURED_CORES_COUNT].name = NULL;
    return element_table;
}

const struct fwk_module_config config_pfdi_monitor = {
    .elements = FWK_MODULE_DYNAMIC_ELEMENTS(get_pfdi_monitor_element_table),
};
