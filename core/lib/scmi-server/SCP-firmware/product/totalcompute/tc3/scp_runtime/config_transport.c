/*
 * Arm SCP/MCP Software
 * Copyright (c) 2023-2024, Arm Limited and Contributors. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include "config_power_domain.h"
#include "scp_mmap.h"
#include "scp_tc_mhu3.h"
#include "tc3_scmi.h"
#include "tc_core.h"

#include <mod_fch_polled.h>
#include <mod_transport.h>

#include <fwk_element.h>
#include <fwk_id.h>
#include <fwk_module.h>
#include <fwk_module_idx.h>

#include <stdint.h>

static const struct fwk_element transport_element_table[] = {
#ifdef BUILD_HAS_MOD_TRANSPORT_FC
    [TC3_TRANSPORT_SCMI_SERVICE_PSCI] = {
#else
    [SCP_TC3_SCMI_SERVICE_IDX_PSCI] = {
#endif
        .name = "PSCI",
        .data = &((
            struct mod_transport_channel_config){
            .channel_type = MOD_TRANSPORT_CHANNEL_TYPE_COMPLETER,
            .policies =
                MOD_TRANSPORT_POLICY_INIT_MAILBOX |
                MOD_TRANSPORT_POLICY_SECURE,
            .out_band_mailbox_address = (uintptr_t)
                SCP_SCMI_PAYLOAD_S_A2P_BASE,
            .out_band_mailbox_size =
                SCP_SCMI_PAYLOAD_SIZE,
            .driver_id = FWK_ID_SUB_ELEMENT_INIT(
                FWK_MODULE_IDX_MHU3,
                SCP_TC_MHU3_DEVICE_IDX_SCP_AP_S,
                0),
            .driver_api_id = FWK_ID_API_INIT(
                FWK_MODULE_IDX_MHU3,
                0),
        }),
     },
#ifdef BUILD_HAS_MOD_TRANSPORT_FC
    [TC3_TRANSPORT_SCMI_SERVICE_OSPM_A2P] = {
#else
    [SCP_TC3_SCMI_SERVICE_IDX_OSPM_A2P] = {
#endif
        .name = "OSPM_A2P",
        .data = &((
            struct mod_transport_channel_config){
            .channel_type =
                MOD_TRANSPORT_CHANNEL_TYPE_COMPLETER,
            .policies =
                MOD_TRANSPORT_POLICY_INIT_MAILBOX,
            .out_band_mailbox_address = (uintptr_t)
                SCP_SCMI_PAYLOAD_NS_A2P_BASE,
            .out_band_mailbox_size =
                SCP_SCMI_PAYLOAD_SIZE,
            .driver_id = FWK_ID_SUB_ELEMENT_INIT(
                FWK_MODULE_IDX_MHU3,
                SCP_TC_MHU3_DEVICE_IDX_SCP_AP_NS,
                0),
            .driver_api_id = FWK_ID_API_INIT(
                FWK_MODULE_IDX_MHU3,
                0),
        }),
    },
#ifdef BUILD_HAS_SCMI_NOTIFICATIONS
#    ifdef BUILD_HAS_MOD_TRANSPORT_FC
    [TC3_TRANSPORT_SCMI_SERVICE_OSPM_P2A] = {
#    else
    [SCP_TC3_SCMI_SERVICE_IDX_OSPM_P2A] = {
#    endif
        .name = "OSPM_P2A",
        .data = &((
            struct mod_transport_channel_config){
           .channel_type =
                MOD_TRANSPORT_CHANNEL_TYPE_REQUESTER,
           .policies =
                MOD_TRANSPORT_POLICY_INIT_MAILBOX,
           .out_band_mailbox_address = (uintptr_t)
                SCP_SCMI_PAYLOAD_NS_P2A_BASE,
           .out_band_mailbox_size =
                SCP_SCMI_PAYLOAD_SIZE,
           .driver_id = FWK_ID_SUB_ELEMENT_INIT(
                FWK_MODULE_IDX_MHU3,
                SCP_TC_MHU3_DEVICE_IDX_SCP_AP_NS,
                1),
           .driver_api_id = FWK_ID_API_INIT(
                FWK_MODULE_IDX_MHU3,
                0),
        }),
    },
#endif
#ifdef BUILD_HAS_MOD_TRANSPORT_FC
    [TC3_TRANSPORT_SCMI_PERF_FCH_GROUP_LITTLE_LEVEL_SET] = {
        .name = "FCH_" TC_GROUP_LITTLE_NAME "_LEVEL_SET",
        .data = &((
            struct mod_transport_channel_config){
            .transport_type =
                MOD_TRANSPORT_CHANNEL_TRANSPORT_TYPE_FAST_CHANNELS,
            .channel_type =
                MOD_TRANSPORT_CHANNEL_TYPE_COMPLETER,
            .driver_id = FWK_ID_ELEMENT_INIT(
                FWK_MODULE_IDX_FCH_POLLED,
                TC3_PLAT_FCH_GROUP_LITTLE_LEVEL_SET),
            .driver_api_id = FWK_ID_API_INIT(
                FWK_MODULE_IDX_FCH_POLLED,
                MOD_FCH_POLLED_API_IDX_TRANSPORT),
        }),
    },
    [TC3_TRANSPORT_SCMI_PERF_FCH_GROUP_LITTLE_LIMIT_SET] = {
        .name = "FCH_" TC_GROUP_LITTLE_NAME "_LIMIT_SET",
        .data = &((
            struct mod_transport_channel_config){
            .transport_type =
                MOD_TRANSPORT_CHANNEL_TRANSPORT_TYPE_FAST_CHANNELS,
            .channel_type =
                MOD_TRANSPORT_CHANNEL_TYPE_COMPLETER,
            .driver_id = FWK_ID_ELEMENT_INIT(
                FWK_MODULE_IDX_FCH_POLLED,
                TC3_PLAT_FCH_GROUP_LITTLE_LIMIT_SET),
            .driver_api_id = FWK_ID_API_INIT(
                FWK_MODULE_IDX_FCH_POLLED,
                MOD_FCH_POLLED_API_IDX_TRANSPORT),
        }),
    },
    [TC3_TRANSPORT_SCMI_PERF_FCH_GROUP_LITTLE_LEVEL_GET] = {
        .name = "FCH_" TC_GROUP_LITTLE_NAME "_LEVEL_GET",
        .data = &((
            struct mod_transport_channel_config){
            .transport_type =
                MOD_TRANSPORT_CHANNEL_TRANSPORT_TYPE_FAST_CHANNELS,
            .channel_type =
                MOD_TRANSPORT_CHANNEL_TYPE_COMPLETER,
            .driver_id = FWK_ID_ELEMENT_INIT(
                FWK_MODULE_IDX_FCH_POLLED,
                TC3_PLAT_FCH_GROUP_LITTLE_LEVEL_GET),
            .driver_api_id = FWK_ID_API_INIT(
                FWK_MODULE_IDX_FCH_POLLED,
                MOD_FCH_POLLED_API_IDX_TRANSPORT),
        }),
    },
    [TC3_TRANSPORT_SCMI_PERF_FCH_GROUP_LITTLE_LIMIT_GET] = {
        .name = "FCH_" TC_GROUP_LITTLE_NAME "_LIMIT_GET",
        .data = &((
            struct mod_transport_channel_config){
            .transport_type =
                MOD_TRANSPORT_CHANNEL_TRANSPORT_TYPE_FAST_CHANNELS,
            .channel_type =
                MOD_TRANSPORT_CHANNEL_TYPE_COMPLETER,
            .driver_id = FWK_ID_ELEMENT_INIT(
                FWK_MODULE_IDX_FCH_POLLED,
                TC3_PLAT_FCH_GROUP_LITTLE_LIMIT_GET),
            .driver_api_id = FWK_ID_API_INIT(
                FWK_MODULE_IDX_FCH_POLLED,
                MOD_FCH_POLLED_API_IDX_TRANSPORT),
        }),
    },
    [TC3_TRANSPORT_SCMI_PERF_FCH_GROUP_MID_LEVEL_SET] = {
        .name = "FCH_" TC_GROUP_MID_NAME "_LEVEL_SET",
        .data = &((
            struct mod_transport_channel_config){
            .transport_type =
                MOD_TRANSPORT_CHANNEL_TRANSPORT_TYPE_FAST_CHANNELS,
            .channel_type =
                MOD_TRANSPORT_CHANNEL_TYPE_COMPLETER,
            .driver_id = FWK_ID_ELEMENT_INIT(
                FWK_MODULE_IDX_FCH_POLLED,
                TC3_PLAT_FCH_GROUP_MID_LEVEL_SET),
            .driver_api_id = FWK_ID_API_INIT(
                FWK_MODULE_IDX_FCH_POLLED,
                MOD_FCH_POLLED_API_IDX_TRANSPORT),
        }),
    },
    [TC3_TRANSPORT_SCMI_PERF_FCH_GROUP_MID_LIMIT_SET] = {
        .name = "FCH_" TC_GROUP_MID_NAME "_LIMIT_SET",
        .data = &((
            struct mod_transport_channel_config){
            .transport_type =
                MOD_TRANSPORT_CHANNEL_TRANSPORT_TYPE_FAST_CHANNELS,
            .channel_type =
                MOD_TRANSPORT_CHANNEL_TYPE_COMPLETER,
            .driver_id = FWK_ID_ELEMENT_INIT(
                FWK_MODULE_IDX_FCH_POLLED,
                TC3_PLAT_FCH_GROUP_MID_LIMIT_SET),
            .driver_api_id = FWK_ID_API_INIT(
                FWK_MODULE_IDX_FCH_POLLED,
                MOD_FCH_POLLED_API_IDX_TRANSPORT),
        }),
    },
    [TC3_TRANSPORT_SCMI_PERF_FCH_GROUP_MID_LEVEL_GET] = {
        .name = "FCH_" TC_GROUP_MID_NAME "_LEVEL_GET",
        .data = &((
            struct mod_transport_channel_config){
            .transport_type =
                MOD_TRANSPORT_CHANNEL_TRANSPORT_TYPE_FAST_CHANNELS,
            .channel_type =
                MOD_TRANSPORT_CHANNEL_TYPE_COMPLETER,
            .driver_id = FWK_ID_ELEMENT_INIT(
                FWK_MODULE_IDX_FCH_POLLED,
                TC3_PLAT_FCH_GROUP_MID_LEVEL_GET),
            .driver_api_id = FWK_ID_API_INIT(
                FWK_MODULE_IDX_FCH_POLLED,
                MOD_FCH_POLLED_API_IDX_TRANSPORT),
        }),
    },
    [TC3_TRANSPORT_SCMI_PERF_FCH_GROUP_MID_LIMIT_GET] = {
        .name = "FCH_" TC_GROUP_MID_NAME "_LIMIT_GET",
        .data = &((
            struct mod_transport_channel_config){
            .transport_type =
                MOD_TRANSPORT_CHANNEL_TRANSPORT_TYPE_FAST_CHANNELS,
            .channel_type =
                MOD_TRANSPORT_CHANNEL_TYPE_COMPLETER,
            .driver_id = FWK_ID_ELEMENT_INIT(
                FWK_MODULE_IDX_FCH_POLLED,
                TC3_PLAT_FCH_GROUP_MID_LIMIT_GET),
            .driver_api_id = FWK_ID_API_INIT(
                FWK_MODULE_IDX_FCH_POLLED,
                MOD_FCH_POLLED_API_IDX_TRANSPORT),
        }),
    },
    [TC3_TRANSPORT_SCMI_PERF_FCH_GROUP_BIG_LEVEL_SET] = {
        .name = "FCH_" TC_GROUP_BIG_NAME "_LEVEL_SET",
        .data = &((
            struct mod_transport_channel_config){
            .transport_type =
                MOD_TRANSPORT_CHANNEL_TRANSPORT_TYPE_FAST_CHANNELS,
            .channel_type =
                MOD_TRANSPORT_CHANNEL_TYPE_COMPLETER,
            .driver_id = FWK_ID_ELEMENT_INIT(
                FWK_MODULE_IDX_FCH_POLLED,
                TC3_PLAT_FCH_GROUP_BIG_LEVEL_SET),
            .driver_api_id = FWK_ID_API_INIT(
                FWK_MODULE_IDX_FCH_POLLED,
                MOD_FCH_POLLED_API_IDX_TRANSPORT),
        }),
    },
    [TC3_TRANSPORT_SCMI_PERF_FCH_GROUP_BIG_LIMIT_SET] = {
        .name = "FCH_" TC_GROUP_BIG_NAME "_LIMIT_SET",
        .data = &((
            struct mod_transport_channel_config){
            .transport_type =
                MOD_TRANSPORT_CHANNEL_TRANSPORT_TYPE_FAST_CHANNELS,
            .channel_type =
                MOD_TRANSPORT_CHANNEL_TYPE_COMPLETER,
            .driver_id = FWK_ID_ELEMENT_INIT(
                FWK_MODULE_IDX_FCH_POLLED,
                TC3_PLAT_FCH_GROUP_BIG_LIMIT_SET),
            .driver_api_id = FWK_ID_API_INIT(
                FWK_MODULE_IDX_FCH_POLLED,
                MOD_FCH_POLLED_API_IDX_TRANSPORT),
        }),
    },
    [TC3_TRANSPORT_SCMI_PERF_FCH_GROUP_BIG_LEVEL_GET] = {
        .name = "FCH_" TC_GROUP_BIG_NAME "_LEVEL_GET",
        .data = &((
            struct mod_transport_channel_config){
            .transport_type =
                MOD_TRANSPORT_CHANNEL_TRANSPORT_TYPE_FAST_CHANNELS,
            .channel_type =
                MOD_TRANSPORT_CHANNEL_TYPE_COMPLETER,
            .driver_id = FWK_ID_ELEMENT_INIT(
                FWK_MODULE_IDX_FCH_POLLED,
                TC3_PLAT_FCH_GROUP_BIG_LEVEL_GET),
            .driver_api_id = FWK_ID_API_INIT(
                FWK_MODULE_IDX_FCH_POLLED,
                MOD_FCH_POLLED_API_IDX_TRANSPORT),
        }),
    },
    [TC3_TRANSPORT_SCMI_PERF_FCH_GROUP_BIG_LIMIT_GET] = {
        .name = "FCH_" TC_GROUP_BIG_NAME "_LIMIT_GET",
        .data = &((
            struct mod_transport_channel_config){
            .transport_type =
                MOD_TRANSPORT_CHANNEL_TRANSPORT_TYPE_FAST_CHANNELS,
            .channel_type =
                MOD_TRANSPORT_CHANNEL_TYPE_COMPLETER,
            .driver_id = FWK_ID_ELEMENT_INIT(
                FWK_MODULE_IDX_FCH_POLLED,
                TC3_PLAT_FCH_GROUP_BIG_LIMIT_GET),
            .driver_api_id = FWK_ID_API_INIT(
                FWK_MODULE_IDX_FCH_POLLED,
                MOD_FCH_POLLED_API_IDX_TRANSPORT),
        }),
    },
    [TC3_TRANSPORT_SCMI_PERF_FCH_GPU_LEVEL_SET] = {
        .name = "FCH_GPU_LEVEL_SET",
        .data = &((
            struct mod_transport_channel_config){
            .transport_type =
                MOD_TRANSPORT_CHANNEL_TRANSPORT_TYPE_FAST_CHANNELS,
            .channel_type =
                MOD_TRANSPORT_CHANNEL_TYPE_COMPLETER,
            .driver_id = FWK_ID_ELEMENT_INIT(
                FWK_MODULE_IDX_FCH_POLLED,
                TC3_PLAT_FCH_GPU_LEVEL_SET),
            .driver_api_id = FWK_ID_API_INIT(
                FWK_MODULE_IDX_FCH_POLLED,
                MOD_FCH_POLLED_API_IDX_TRANSPORT),
        }),
    },
    [TC3_TRANSPORT_SCMI_PERF_FCH_GPU_LIMIT_SET] = {
        .name = "FCH_GPU_LIMIT_SET",
        .data = &((
            struct mod_transport_channel_config){
            .transport_type =
                MOD_TRANSPORT_CHANNEL_TRANSPORT_TYPE_FAST_CHANNELS,
            .channel_type =
                MOD_TRANSPORT_CHANNEL_TYPE_COMPLETER,
            .driver_id = FWK_ID_ELEMENT_INIT(
                FWK_MODULE_IDX_FCH_POLLED,
                TC3_PLAT_FCH_GPU_LIMIT_SET),
            .driver_api_id = FWK_ID_API_INIT(
                FWK_MODULE_IDX_FCH_POLLED,
                MOD_FCH_POLLED_API_IDX_TRANSPORT),
        }),
    },
    [TC3_TRANSPORT_SCMI_PERF_FCH_GPU_LEVEL_GET] = {
        .name = "FCH_GPU_LEVEL_GET",
        .data = &((
            struct mod_transport_channel_config){
            .transport_type =
                MOD_TRANSPORT_CHANNEL_TRANSPORT_TYPE_FAST_CHANNELS,
            .channel_type =
                MOD_TRANSPORT_CHANNEL_TYPE_COMPLETER,
            .driver_id = FWK_ID_ELEMENT_INIT(
                FWK_MODULE_IDX_FCH_POLLED,
                TC3_PLAT_FCH_GPU_LEVEL_GET),
            .driver_api_id = FWK_ID_API_INIT(
                FWK_MODULE_IDX_FCH_POLLED,
                MOD_FCH_POLLED_API_IDX_TRANSPORT),
        }),
    },
    [TC3_TRANSPORT_SCMI_PERF_FCH_GPU_LIMIT_GET] = {
        .name = "FCH_GPU_LIMIT_GET",
        .data = &((
            struct mod_transport_channel_config){
            .transport_type =
                MOD_TRANSPORT_CHANNEL_TRANSPORT_TYPE_FAST_CHANNELS,
            .channel_type =
                MOD_TRANSPORT_CHANNEL_TYPE_COMPLETER,
            .driver_id = FWK_ID_ELEMENT_INIT(
                FWK_MODULE_IDX_FCH_POLLED,
                TC3_PLAT_FCH_GPU_LIMIT_GET),
            .driver_api_id = FWK_ID_API_INIT(
                FWK_MODULE_IDX_FCH_POLLED,
                MOD_FCH_POLLED_API_IDX_TRANSPORT),
        }),
    },
#endif
    { 0 },
};

static const struct fwk_element *transport_get_element_table(fwk_id_t module_id)
{
    unsigned int idx;
    struct mod_transport_channel_config *config;

    for (idx = 0; idx < FWK_ARRAY_SIZE(transport_element_table) - 1; idx++) {
        config =
            (struct mod_transport_channel_config *)(transport_element_table[idx]
                                                        .data);
        config->pd_source_id = FWK_ID_ELEMENT(
            FWK_MODULE_IDX_POWER_DOMAIN,
            TC_NUMBER_OF_CORES + TC_NUMBER_OF_CLUSTERS +
                PD_STATIC_DEV_IDX_SYSTOP);
    }

    return transport_element_table;
}

const struct fwk_module_config config_transport = {
    .elements = FWK_MODULE_DYNAMIC_ELEMENTS(transport_get_element_table),
};
