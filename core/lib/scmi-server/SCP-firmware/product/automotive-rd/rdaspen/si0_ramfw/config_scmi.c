/*
 * Arm SCP/MCP Software
 * Copyright (c) 2025, Arm Limited and Contributors. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 * Description:
 *     Configuration data for module 'scmi'.
 */

#include "si0_cfgd_scmi.h"
#include "si0_cfgd_transport.h"

#include <mod_scmi.h>
#include <mod_transport.h>

#include <fwk_element.h>
#include <fwk_id.h>
#include <fwk_macros.h>
#include <fwk_module.h>
#include <fwk_module_idx.h>

#define SCMI_PFDI_MONITOR_AP(cluster, core) \
    { \
        .name = "SCMI_PFDI_MONITOR_AP_CLUSTER_" #cluster "_CORE_" #core, \
        .data = &((struct mod_scmi_service_config){ \
            .transport_id = FWK_ID_ELEMENT_INIT( \
                FWK_MODULE_IDX_TRANSPORT, \
                SI0_CFGD_MOD_TRANSPORT_EIDX_PFDI_MONITOR_AP_CLUSTER_##cluster##_CORE_##core), \
            .transport_api_id = FWK_ID_API_INIT( \
                FWK_MODULE_IDX_TRANSPORT, \
                MOD_TRANSPORT_API_IDX_SCMI_TO_TRANSPORT), \
            .transport_notification_init_id = FWK_ID_NOTIFICATION_INIT( \
                FWK_MODULE_IDX_TRANSPORT, \
                MOD_TRANSPORT_NOTIFICATION_IDX_INITIALIZED), \
            .scmi_agent_id = SI0_SCMI_AGENT_IDX_PFDI_MONITOR, \
            .scmi_p2a_id = FWK_ID_NONE_INIT, \
        }), \
    }

static const struct fwk_element service_table[SI0_CFGD_MOD_SCMI_EIDX_COUNT + 1] = {
    [SI0_CFGD_MOD_SCMI_EIDX_RSE] = {
        .name = "SCMI_SERVICE_TO_RSE",
        .data = &((struct mod_scmi_service_config) {
            .transport_id = FWK_ID_ELEMENT_INIT(
                FWK_MODULE_IDX_TRANSPORT,
                SI0_CFGD_MOD_TRANSPORT_EIDX_SCMI_RSE),
            .transport_api_id = FWK_ID_API_INIT(
                FWK_MODULE_IDX_TRANSPORT,
                MOD_TRANSPORT_API_IDX_SCMI_TO_TRANSPORT),
            .transport_notification_init_id = FWK_ID_NOTIFICATION_INIT(
                FWK_MODULE_IDX_TRANSPORT,
                MOD_TRANSPORT_NOTIFICATION_IDX_INITIALIZED),
            .scmi_agent_id = SI0_SCMI_AGENT_IDX_RSE,
#ifdef BUILD_HAS_SCMI_NOTIFICATIONS
            .scmi_p2a_id = FWK_ID_ELEMENT_INIT(
                FWK_MODULE_IDX_SCMI,
                SI0_CFGD_MOD_SCMI_EIDX_RSE_P2A),
#else
            .scmi_p2a_id = FWK_ID_NONE_INIT,
#endif
        }),
    },
    [SI0_CFGD_MOD_SCMI_EIDX_PSCI] = {
        .name = "SERVICE0",
        .data = &((struct mod_scmi_service_config) {
            .transport_id = FWK_ID_ELEMENT_INIT(
                FWK_MODULE_IDX_TRANSPORT,
                SI0_CFGD_MOD_TRANSPORT_EIDX_PSCI),
            .transport_api_id = FWK_ID_API_INIT(
                FWK_MODULE_IDX_TRANSPORT,
                MOD_TRANSPORT_API_IDX_SCMI_TO_TRANSPORT),
            .transport_notification_init_id = FWK_ID_NOTIFICATION_INIT(
                FWK_MODULE_IDX_TRANSPORT,
                MOD_TRANSPORT_NOTIFICATION_IDX_INITIALIZED),
            .scmi_agent_id = SI0_SCMI_AGENT_IDX_PSCI,
            .scmi_p2a_id = FWK_ID_NONE_INIT,

        }),
    },
#ifdef BUILD_HAS_SCMI_NOTIFICATIONS
    [SI0_CFGD_MOD_SCMI_EIDX_RSE_P2A] = {
        .name = "SCMI_SERVICE_2_RSE_P2A",
        .data = &((struct mod_scmi_service_config) {
            .transport_id = FWK_ID_ELEMENT_INIT(
                FWK_MODULE_IDX_TRANSPORT,
                SI0_CFGD_MOD_TRANSPORT_EIDX_SCMI_RSE_P2A),
            .transport_api_id = FWK_ID_API_INIT(
                FWK_MODULE_IDX_TRANSPORT,
                MOD_TRANSPORT_API_IDX_SCMI_TO_TRANSPORT),
            .transport_notification_init_id = FWK_ID_NONE_INIT,
            .scmi_agent_id = SI0_SCMI_AGENT_IDX_RSE,
            .scmi_p2a_id = FWK_ID_NONE_INIT,
        }),
    },
#endif
    [SI0_CFGD_MOD_SCMI_EIDX_PFDI_MONITOR_AP_CLUSTER_0_CORE_0] = SCMI_PFDI_MONITOR_AP(0, 0),
    [SI0_CFGD_MOD_SCMI_EIDX_PFDI_MONITOR_AP_CLUSTER_0_CORE_1] = SCMI_PFDI_MONITOR_AP(0, 1),
    [SI0_CFGD_MOD_SCMI_EIDX_PFDI_MONITOR_AP_CLUSTER_0_CORE_2] = SCMI_PFDI_MONITOR_AP(0, 2),
    [SI0_CFGD_MOD_SCMI_EIDX_PFDI_MONITOR_AP_CLUSTER_0_CORE_3] = SCMI_PFDI_MONITOR_AP(0, 3),
    [SI0_CFGD_MOD_SCMI_EIDX_PFDI_MONITOR_AP_CLUSTER_1_CORE_0] = SCMI_PFDI_MONITOR_AP(1, 0),
    [SI0_CFGD_MOD_SCMI_EIDX_PFDI_MONITOR_AP_CLUSTER_1_CORE_1] = SCMI_PFDI_MONITOR_AP(1, 1),
    [SI0_CFGD_MOD_SCMI_EIDX_PFDI_MONITOR_AP_CLUSTER_1_CORE_2] = SCMI_PFDI_MONITOR_AP(1, 2),
    [SI0_CFGD_MOD_SCMI_EIDX_PFDI_MONITOR_AP_CLUSTER_1_CORE_3] = SCMI_PFDI_MONITOR_AP(1, 3),
    [SI0_CFGD_MOD_SCMI_EIDX_PFDI_MONITOR_AP_CLUSTER_2_CORE_0] = SCMI_PFDI_MONITOR_AP(2, 0),
    [SI0_CFGD_MOD_SCMI_EIDX_PFDI_MONITOR_AP_CLUSTER_2_CORE_1] = SCMI_PFDI_MONITOR_AP(2, 1),
    [SI0_CFGD_MOD_SCMI_EIDX_PFDI_MONITOR_AP_CLUSTER_2_CORE_2] = SCMI_PFDI_MONITOR_AP(2, 2),
    [SI0_CFGD_MOD_SCMI_EIDX_PFDI_MONITOR_AP_CLUSTER_2_CORE_3] = SCMI_PFDI_MONITOR_AP(2, 3),
    [SI0_CFGD_MOD_SCMI_EIDX_PFDI_MONITOR_AP_CLUSTER_3_CORE_0] = SCMI_PFDI_MONITOR_AP(3, 0),
    [SI0_CFGD_MOD_SCMI_EIDX_PFDI_MONITOR_AP_CLUSTER_3_CORE_1] = SCMI_PFDI_MONITOR_AP(3, 1),
    [SI0_CFGD_MOD_SCMI_EIDX_PFDI_MONITOR_AP_CLUSTER_3_CORE_2] = SCMI_PFDI_MONITOR_AP(3, 2),
    [SI0_CFGD_MOD_SCMI_EIDX_PFDI_MONITOR_AP_CLUSTER_3_CORE_3] = SCMI_PFDI_MONITOR_AP(3, 3),
    [SI0_CFGD_MOD_SCMI_EIDX_COUNT] = { 0 }
};

static const struct fwk_element *get_service_table(fwk_id_t module_id)
{
    return service_table;
}

static struct mod_scmi_agent agent_table[SI0_SCMI_AGENT_IDX_COUNT] = {
    [SI0_SCMI_AGENT_IDX_RSE] = {
        .type = SCMI_AGENT_TYPE_PSCI,
        .name = "SI0_SCMI_AGENT_RSE",
    },
    [SI0_SCMI_AGENT_IDX_PSCI] = {
        .type = SCMI_AGENT_TYPE_PSCI,
        .name = "PSCI",
    },
    [SI0_SCMI_AGENT_IDX_PFDI_MONITOR] = {
        .type = SCMI_AGENT_TYPE_OTHER,
        .name = "SI0_SCMI_AGENT_PFDI_MONITOR",
    },
};

const struct fwk_module_config config_scmi = {
    .data =
        &(struct mod_scmi_config){
            .protocol_count_max = 5,
            .protocol_requester_count_max = 2,
            .agent_count = FWK_ARRAY_SIZE(agent_table) - 1,
            .agent_table = agent_table,
            .vendor_identifier = "arm",
            .sub_vendor_identifier = "arm",
        },

    .elements = FWK_MODULE_DYNAMIC_ELEMENTS(get_service_table),
};
