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
#include "si0_cfgd_scmi.h"

#include <mod_scmi_pfdi_monitor.h>

#include <fwk_id.h>
#include <fwk_module.h>
#include <fwk_module_idx.h>

#define SCMI_PFDI_MONITOR_AP(cluster, core) \
    { \
        .name = "AP cluster " #cluster " core " #core, \
        .data = &((const struct mod_scmi_pfdi_monitor_core_config){ \
            .scmi_service_id = FWK_ID_ELEMENT_INIT( \
                FWK_MODULE_IDX_SCMI, \
                SI0_CFGD_MOD_SCMI_EIDX_PFDI_MONITOR_AP_CLUSTER_##cluster##_CORE_##core), \
            .pfdi_monitor_id = FWK_ID_ELEMENT_INIT( \
                FWK_MODULE_IDX_PFDI_MONITOR, \
                SI0_CFGD_MOD_PFDI_MONITOR_EIDX_AP_CLUSTER_##cluster##_CORE_##core), \
        }), \
    }

static const struct fwk_element element_table[] = {
    SCMI_PFDI_MONITOR_AP(0, 0),
    SCMI_PFDI_MONITOR_AP(0, 1),
    SCMI_PFDI_MONITOR_AP(0, 2),
    SCMI_PFDI_MONITOR_AP(0, 3),
    SCMI_PFDI_MONITOR_AP(1, 0),
    SCMI_PFDI_MONITOR_AP(1, 1),
    SCMI_PFDI_MONITOR_AP(1, 2),
    SCMI_PFDI_MONITOR_AP(1, 3),
    SCMI_PFDI_MONITOR_AP(2, 0),
    SCMI_PFDI_MONITOR_AP(2, 1),
    SCMI_PFDI_MONITOR_AP(2, 2),
    SCMI_PFDI_MONITOR_AP(2, 3),
    SCMI_PFDI_MONITOR_AP(3, 0),
    SCMI_PFDI_MONITOR_AP(3, 1),
    SCMI_PFDI_MONITOR_AP(3, 2),
    SCMI_PFDI_MONITOR_AP(3, 3),
    { 0 },
};

static const struct fwk_element *get_scmi_pfdi_monitor_element_table(
    fwk_id_t unused)
{
    return element_table;
}

const struct fwk_module_config config_scmi_pfdi_monitor = {
    .elements =
        FWK_MODULE_DYNAMIC_ELEMENTS(get_scmi_pfdi_monitor_element_table),
};
