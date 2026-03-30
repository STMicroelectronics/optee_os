/*
 * Arm SCP/MCP Software
 * Copyright (c) 2024-2025, Arm Limited and Contributors. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 * Description:
 *     Configuration data for module 'scmi'.
 */

#include "scp_cfgd_scmi.h"
#include "scp_cfgd_transport.h"

#include <mod_scmi.h>
#include <mod_transport.h>

#include <fwk_element.h>
#include <fwk_id.h>
#include <fwk_macros.h>
#include <fwk_module.h>
#include <fwk_module_idx.h>

/* Module 'scmi' element count */
#define MOD_SCMI_ELEMENT_COUNT (SCP_CFGD_MOD_SCMI_EIDX_COUNT + 1)

static const struct fwk_element service_table[MOD_SCMI_ELEMENT_COUNT] = {
    [SCP_CFGD_MOD_SCMI_EIDX_PSCI] = {
        .name = "SERVICE0",
        .data = &((struct mod_scmi_service_config) {
            .transport_id = FWK_ID_ELEMENT_INIT(
                FWK_MODULE_IDX_TRANSPORT,
                SCP_CFGD_MOD_TRANSPORT_EIDX_PSCI),
            .transport_api_id = FWK_ID_API_INIT(
                FWK_MODULE_IDX_TRANSPORT,
                MOD_TRANSPORT_API_IDX_SCMI_TO_TRANSPORT),
            .transport_notification_init_id = FWK_ID_NOTIFICATION_INIT(
                FWK_MODULE_IDX_TRANSPORT,
                MOD_TRANSPORT_NOTIFICATION_IDX_INITIALIZED),
            .scmi_agent_id = SCP_SCMI_AGENT_IDX_PSCI,
            .scmi_p2a_id = FWK_ID_NONE_INIT,
        }),
    },
    [SCP_CFGD_MOD_SCMI_EIDX_RSE_SCMI_SEND] = {
        .name = "SCP_RSE_SCMI_SEND",
        .data = &((struct mod_scmi_service_config) {
            .transport_id = FWK_ID_ELEMENT_INIT(
                FWK_MODULE_IDX_TRANSPORT,
                SCP_CFGD_MOD_TRANSPORT_EIDX_RSE_SCMI_MSG_SEND_CH),
            .transport_api_id = FWK_ID_API_INIT(
                FWK_MODULE_IDX_TRANSPORT,
                MOD_TRANSPORT_API_IDX_SCMI_TO_TRANSPORT),
            .transport_notification_init_id = FWK_ID_NOTIFICATION_INIT(
                FWK_MODULE_IDX_TRANSPORT,
                MOD_TRANSPORT_NOTIFICATION_IDX_INITIALIZED),
            .scmi_agent_id = SCP_SCMI_AGENT_IDX_RSE,
            .scmi_p2a_id = FWK_ID_NONE_INIT,
        }),
    },
    [SCP_CFGD_MOD_SCMI_EIDX_MCP_SCMI_SEND] = {
        .name = "SCP_MCP_SCMI_SEND",
        .data = &((struct mod_scmi_service_config) {
            .transport_id = FWK_ID_ELEMENT_INIT(
                FWK_MODULE_IDX_TRANSPORT,
                SCP_CFGD_MOD_TRANSPORT_EIDX_MCP_SCMI_MSG_SEND_CH),
            .transport_api_id = FWK_ID_API_INIT(
                FWK_MODULE_IDX_TRANSPORT,
                MOD_TRANSPORT_API_IDX_SCMI_TO_TRANSPORT),
            .transport_notification_init_id = FWK_ID_NOTIFICATION_INIT(
                FWK_MODULE_IDX_TRANSPORT,
                MOD_TRANSPORT_NOTIFICATION_IDX_INITIALIZED),
            .scmi_agent_id = SCP_SCMI_AGENT_IDX_MCP,
            .scmi_p2a_id = FWK_ID_NONE_INIT,
        }),
    },
#ifdef BUILD_HAS_SCMI_NOTIFICATIONS
    [SCP_CFGD_MOD_SCMI_EIDX_RSE_SCMI_RECV] = {
        .name = "SCP_RSE_SCMI_RECV",
        .data = &((struct mod_scmi_service_config) {
            .transport_id = FWK_ID_ELEMENT_INIT(
                FWK_MODULE_IDX_TRANSPORT,
                SCP_CFGD_MOD_TRANSPORT_EIDX_RSE_SCMI_MSG_RECV_CH),
            .transport_api_id = FWK_ID_API_INIT(
                FWK_MODULE_IDX_TRANSPORT,
                MOD_TRANSPORT_API_IDX_SCMI_TO_TRANSPORT),
            .transport_notification_init_id = FWK_ID_NOTIFICATION_INIT(
                FWK_MODULE_IDX_TRANSPORT,
                MOD_TRANSPORT_NOTIFICATION_IDX_INITIALIZED),
            .scmi_agent_id = SCP_SCMI_AGENT_IDX_RSE,
            .scmi_p2a_id = FWK_ID_ELEMENT_INIT(
                FWK_MODULE_IDX_SCMI,
                SCP_CFGD_MOD_SCMI_EIDX_RSE_SCMI_SEND),
        }),
    },
    [SCP_CFGD_MOD_SCMI_EIDX_MCP_SCMI_RECV] = {
        .name = "SCP_MCP_SCMI_RECV",
        .data = &((struct mod_scmi_service_config) {
            .transport_id = FWK_ID_ELEMENT_INIT(
                FWK_MODULE_IDX_TRANSPORT,
                SCP_CFGD_MOD_TRANSPORT_EIDX_MCP_SCMI_MSG_RECV_CH),
            .transport_api_id = FWK_ID_API_INIT(
                FWK_MODULE_IDX_TRANSPORT,
                MOD_TRANSPORT_API_IDX_SCMI_TO_TRANSPORT),
            .transport_notification_init_id = FWK_ID_NOTIFICATION_INIT(
                FWK_MODULE_IDX_TRANSPORT,
                MOD_TRANSPORT_NOTIFICATION_IDX_INITIALIZED),
            .scmi_agent_id = SCP_SCMI_AGENT_IDX_MCP,
            .scmi_p2a_id = FWK_ID_ELEMENT_INIT(
                FWK_MODULE_IDX_SCMI,
                SCP_CFGD_MOD_SCMI_EIDX_MCP_SCMI_SEND),
        }),
    },
#endif
    [SCP_CFGD_MOD_SCMI_EIDX_COUNT] = { 0 }
};

static const struct fwk_element *get_service_table(fwk_id_t module_id)
{
    return service_table;
}

static struct mod_scmi_agent agent_table[SCP_SCMI_AGENT_IDX_COUNT] = {
    [SCP_SCMI_AGENT_IDX_PSCI] = {
        .type = SCMI_AGENT_TYPE_PSCI,
        .name = "PSCI",
    },
    [SCP_SCMI_AGENT_IDX_RSE] = {
        .type = SCMI_AGENT_TYPE_MANAGEMENT,
        .name = "RSE",
    },
    [SCP_SCMI_AGENT_IDX_MCP] = {
        .type = SCMI_AGENT_TYPE_MANAGEMENT,
        .name = "MCP",
    },
};

const struct fwk_module_config config_scmi = {
    .data =
        &(struct mod_scmi_config){
            .protocol_count_max = 4,
            .protocol_requester_count_max = SCMI_PROTOCOL_REQUESTER_COUNT,
            .agent_count = FWK_ARRAY_SIZE(agent_table) - 1,
            .agent_table = agent_table,
            .vendor_identifier = "arm",
            .sub_vendor_identifier = "arm",
        },

    .elements = FWK_MODULE_DYNAMIC_ELEMENTS(get_service_table),
};
