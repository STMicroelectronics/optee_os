/*
 * Arm SCP/MCP Software
 * Copyright (c) 2024-2025, Arm Limited and Contributors. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 * Description:
 *     Configuration data for module 'transport'.
 */

#include "mcp_cfgd_mhu3.h"
#include "mcp_cfgd_transport.h"
#include "mcp_fw_mmap.h"
#include "platform_variant.h"

#include <mod_mhu3.h>
#include <mod_scmi.h>
#include <mod_transport.h>

#include <fwk_element.h>
#include <fwk_id.h>
#include <fwk_module.h>
#include <fwk_module_idx.h>

#include <stdint.h>

static const struct fwk_element transport_element_table[] = {
    [MCP_CFGD_MOD_TRANSPORT_EIDX_SCP_SCMI_MSG_SEND_CH] = {
        .name = "MCP_SCP_SCMI_MSG_SEND_CH",
        .data = &((
            struct mod_transport_channel_config) {
                .transport_type = MOD_TRANSPORT_CHANNEL_TRANSPORT_TYPE_OUT_BAND,
#if (PLATFORM_VARIANT == RD_V3_CFG2)
                /* TODO: Enable shutdown/reboot support for multichip platform
                 * as well, once chip to chip SCP communicaiton is enabled */
                .policies = MOD_TRANSPORT_POLICY_NONE,
#else
                .policies = MOD_TRANSPORT_POLICY_INIT_MAILBOX |
                    MOD_TRANSPORT_POLICY_SECURE,
#endif
                .channel_type = MOD_TRANSPORT_CHANNEL_TYPE_REQUESTER,
                .out_band_mailbox_address =
                    (uintptr_t) MCP_SCP_SCMI_MSG_PAYLOAD_BASE,
                .out_band_mailbox_size = SCP_MCP_SCMI_MSG_PAYLOAD_SIZE,
                .signal_api_id =
                    FWK_ID_API_INIT(
                        FWK_MODULE_IDX_SCMI,
                        MOD_SCMI_API_IDX_TRANSPORT),
                .driver_id =
                    FWK_ID_SUB_ELEMENT_INIT(
                        FWK_MODULE_IDX_MHU3,
                        MCP_CFGD_MOD_MHU3_EIDX_MCP_SCP_S,
                        0),
                .driver_api_id =
                    FWK_ID_API_INIT(
                        FWK_MODULE_IDX_MHU3,
                        MOD_MHU3_API_IDX_TRANSPORT_DRIVER),
        }),
    },
    [MCP_CFGD_MOD_TRANSPORT_EIDX_SCP_SCMI_MSG_RECV_CH] = {
        .name = "MCP_SCP_SCMI_MSG_RECV_CH",
        .data = &((
            struct mod_transport_channel_config) {
                .transport_type = MOD_TRANSPORT_CHANNEL_TRANSPORT_TYPE_OUT_BAND,
#if (PLATFORM_VARIANT == RD_V3_CFG2)
                .policies = MOD_TRANSPORT_POLICY_NONE,
#else
                .policies = MOD_TRANSPORT_POLICY_INIT_MAILBOX |
                    MOD_TRANSPORT_POLICY_SECURE,
#endif
                .channel_type = MOD_TRANSPORT_CHANNEL_TYPE_COMPLETER,
                .out_band_mailbox_address =
                    (uintptr_t) MCP_SCP_SCMI_MSG_PAYLOAD_BASE,
                .out_band_mailbox_size = SCP_MCP_SCMI_MSG_PAYLOAD_SIZE,
                .driver_id =
                    FWK_ID_SUB_ELEMENT_INIT(
                        FWK_MODULE_IDX_MHU3,
                        MCP_CFGD_MOD_MHU3_EIDX_MCP_SCP_S,
                        0),
                .driver_api_id =
                    FWK_ID_API_INIT(
                        FWK_MODULE_IDX_MHU3,
                        MOD_MHU3_API_IDX_TRANSPORT_DRIVER),
        }),
    },
    [MCP_CFGD_MOD_TRANSPORT_EIDX_COUNT] = { 0 },
};

static const struct fwk_element *transport_get_element_table(fwk_id_t module_id)
{
    return transport_element_table;
}

const struct fwk_module_config config_transport = {
    .elements = FWK_MODULE_DYNAMIC_ELEMENTS(transport_get_element_table),
};
