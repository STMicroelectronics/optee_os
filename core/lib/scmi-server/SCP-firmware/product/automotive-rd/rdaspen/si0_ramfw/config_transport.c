/*
 * Arm SCP/MCP Software
 * Copyright (c) 2025, Arm Limited and Contributors. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 * Description:
 *     Configuration data for module 'transport'.
 */

#include "platform_core.h"
#include "si0_cfgd_mhu3.h"
#include "si0_cfgd_power_domain.h"
#include "si0_cfgd_transport.h"
#include "si0_mmap.h"

#include <mod_mhu3.h>
#include <mod_si0_platform.h>
#include <mod_transport.h>

#include <fwk_element.h>
#include <fwk_id.h>
#include <fwk_module.h>
#include <fwk_module_idx.h>

/* Secure transport channel with mailbox initialization policy */
#define TRANSPORT_CH_SEC_MBX_INIT \
    (MOD_TRANSPORT_POLICY_INIT_MAILBOX | MOD_TRANSPORT_POLICY_SECURE)

/* Subsystem initialized notification id (platform notification) */
#define PLATFORM_SI0_NOTIFICATION_ID \
    FWK_ID_NOTIFICATION_INIT( \
        FWK_MODULE_IDX_SI0_PLATFORM, \
        MOD_SI0_PLATFORM_NOTIFICATION_IDX_SUBSYS_INITIALIZED)

#define TRANSPORT_PFDI_MONITOR_AP(cluster, core) \
    { \
        .name = "TRANSPORT_PFDI_MONITOR_AP_CLUSTER_" #cluster "_CORE_" #core, \
        .data = &((struct mod_transport_channel_config){ \
            .transport_type = MOD_TRANSPORT_CHANNEL_TRANSPORT_TYPE_OUT_BAND, \
            .policies = TRANSPORT_CH_SEC_MBX_INIT, \
            .channel_type = MOD_TRANSPORT_CHANNEL_TYPE_COMPLETER, \
            .out_band_mailbox_address = \
                (uintptr_t)SI0_SCMI_PFDI_MONITOR_S_A2P_BASE + \
                (((cluster * CORES_PER_CLUSTER) + core) * \
                 SI0_SCMI_PFDI_MONITOR_SIZE_CORE), \
            .out_band_mailbox_size = SI0_SCMI_PFDI_MONITOR_SIZE_CORE, \
            .driver_id = FWK_ID_SUB_ELEMENT_INIT( \
                FWK_MODULE_IDX_MHU3, \
                SI0_CFGD_MOD_MHU3_EIDX_SI0_AP_S, \
                SI0_CFGD_MOD_MHU3_SI0_AP_S_EIDX_PFDI_MONITOR_AP_CLUSTER_##cluster##_CORE_##core), \
            .driver_api_id = FWK_ID_API_INIT( \
                FWK_MODULE_IDX_MHU3, MOD_MHU3_API_IDX_TRANSPORT_DRIVER), \
        }), \
    }

/* Module 'transport' element configuration table */
static const struct fwk_element element_table[]  = {
    [SI0_CFGD_MOD_TRANSPORT_EIDX_SCMI_RSE] = {
        .name = "SI0_RSE_SCMI_TRANSPORT",
        .data = &((
            struct mod_transport_channel_config) {
                .transport_type = MOD_TRANSPORT_CHANNEL_TRANSPORT_TYPE_OUT_BAND,
                .policies = TRANSPORT_CH_SEC_MBX_INIT,
                .channel_type = MOD_TRANSPORT_CHANNEL_TYPE_COMPLETER,
                .out_band_mailbox_address =
                    (uintptr_t) SI0_RSE_SCMI_PAYLOAD_BASE,
                .out_band_mailbox_size = SI0_SCMI_PAYLOAD_SIZE,
                .driver_id =
                    FWK_ID_SUB_ELEMENT_INIT(
                        FWK_MODULE_IDX_MHU3,
                        SI0_CFGD_MOD_MHU3_EIDX_SI0_RSE,
                        0),
                .driver_api_id =
                    FWK_ID_API_INIT(
                        FWK_MODULE_IDX_MHU3,
                        MOD_MHU3_API_IDX_TRANSPORT_DRIVER),
                .platform_notification = {
                    .notification_id = FWK_ID_NONE,
                    .source_id = FWK_ID_NONE,
                },
        }),
    },
    [SI0_CFGD_MOD_TRANSPORT_EIDX_PSCI] = {
        .name = "PSCI",
        .data = &((
            struct mod_transport_channel_config) {
                .transport_type = MOD_TRANSPORT_CHANNEL_TRANSPORT_TYPE_OUT_BAND,
                .policies = TRANSPORT_CH_SEC_MBX_INIT,
                .channel_type = MOD_TRANSPORT_CHANNEL_TYPE_COMPLETER,
                .out_band_mailbox_address =
                    (uintptr_t) SI0_SCMI_PAYLOAD_S_A2P_BASE,
                .out_band_mailbox_size = SI0_SCMI_PAYLOAD_SIZE,
                .driver_id =
                    FWK_ID_SUB_ELEMENT_INIT(
                        FWK_MODULE_IDX_MHU3,
                        SI0_CFGD_MOD_MHU3_EIDX_SI0_AP_S,
                        0),
                .driver_api_id =
                    FWK_ID_API_INIT(
                        FWK_MODULE_IDX_MHU3,
                        MOD_MHU3_API_IDX_TRANSPORT_DRIVER),
                .platform_notification = {
                    .notification_id = PLATFORM_SI0_NOTIFICATION_ID,
                    .source_id = FWK_ID_MODULE_INIT(
                        FWK_MODULE_IDX_SI0_PLATFORM),
                },
        }),
    },
#ifdef BUILD_HAS_SCMI_NOTIFICATIONS
    [SI0_CFGD_MOD_TRANSPORT_EIDX_SCMI_RSE_P2A] = {
        .name = "SI0_RSE_SCMI_TRANSPORT_P2A",
        .data = &((
            struct mod_transport_channel_config) {
                .transport_type = MOD_TRANSPORT_CHANNEL_TRANSPORT_TYPE_OUT_BAND,
                .policies = TRANSPORT_CH_SEC_MBX_INIT,
                .channel_type = MOD_TRANSPORT_CHANNEL_TYPE_COMPLETER,
                .out_band_mailbox_address =
                    (uintptr_t) SI0_RSE_SCMI_P2A_PAYLOAD_BASE,
                .out_band_mailbox_size = SI0_SCMI_PAYLOAD_SIZE,
                .driver_id =
                    FWK_ID_SUB_ELEMENT_INIT(
                        FWK_MODULE_IDX_MHU3,
                        SI0_CFGD_MOD_MHU3_EIDX_SI0_RSE,
                        1),
                .driver_api_id =
                    FWK_ID_API_INIT(
                        FWK_MODULE_IDX_MHU3,
                        MOD_MHU3_API_IDX_TRANSPORT_DRIVER),
        }),
    },
#endif
    [SI0_CFGD_MOD_TRANSPORT_EIDX_COUNT] = { 0 },
};

const struct fwk_module_config config_transport = {
    .elements = FWK_MODULE_STATIC_ELEMENTS_PTR(element_table),
};
