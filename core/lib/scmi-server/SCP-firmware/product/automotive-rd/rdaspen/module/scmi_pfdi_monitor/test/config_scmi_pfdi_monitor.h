/*
 * Arm SCP/MCP Software
 * Copyright (c) 2025, Arm Limited and Contributors. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 * Description:
 *     Configuration data for module 'scmi_pfdi_monitor'.
 */

#include <mod_scmi_pfdi_monitor.h>

#include <fwk_id.h>
#include <fwk_module.h>
#include <fwk_module_idx.h>

enum si0_mod_scmi_pfdi_monitor_element_idx {
    SI0_MOD_SCMI_PFDI_MONITOR_EIDX_CORE_0,
    SI0_MOD_SCMI_PFDI_MONITOR_EIDX_CORE_1,
    SI0_MOD_SCMI_PFDI_MONITOR_EIDX_COUNT,
};

#define SCMI_PFDI_MONITOR(core) \
    { \
        .name = "Core " #core, \
        .data = &((const struct mod_scmi_pfdi_monitor_core_config){ \
            .scmi_service_id = FWK_ID_ELEMENT_INIT( \
                FWK_MODULE_IDX_SCMI, \
                SI0_MOD_SCMI_PFDI_MONITOR_EIDX_CORE_##core), \
            .pfdi_monitor_id = FWK_ID_ELEMENT_INIT( \
                FWK_MODULE_IDX_PFDI_MONITOR, \
                SI0_MOD_SCMI_PFDI_MONITOR_EIDX_CORE_##core), \
        }), \
    }

static const struct fwk_element element_table[] = {
    [SI0_MOD_SCMI_PFDI_MONITOR_EIDX_CORE_0] = SCMI_PFDI_MONITOR(0),
    [SI0_MOD_SCMI_PFDI_MONITOR_EIDX_CORE_1] = SCMI_PFDI_MONITOR(1),
    [SI0_MOD_SCMI_PFDI_MONITOR_EIDX_COUNT] = { 0 },
};
