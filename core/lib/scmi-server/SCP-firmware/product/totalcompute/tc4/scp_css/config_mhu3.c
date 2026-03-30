/*
 * Arm SCP/MCP Software
 * Copyright (c) 2023-2024, Arm Limited and Contributors. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */
#include "scp_mmap.h"
#include "scp_tc_mhu3.h"

#include <mod_mhu3.h>

#include <fwk_element.h>
#include <fwk_id.h>
#include <fwk_module.h>

#include <fmw_cmsis.h>

#include <stddef.h>

struct mod_mhu3_channel_config ap_s_channels[] = {
    MOD_MHU3_INIT_DBCH(
        /* PBX CH */ 0,
        /* FLAG */ 0,
        /* MBX CH */ 0,
        /* FLAG */ 0),
};
struct mod_mhu3_channel_config ap_ns_channels[] = {
    MOD_MHU3_INIT_DBCH(
        /* PBX CH */ 0,
        /* FLAG */ 0,
        /* MBX CH */ 0,
        /* FLAG */ 0),
    MOD_MHU3_INIT_DBCH(
        /* PBX CH */ 0,
        /* FLAG */ 1,
        /* MBX CH */ 0,
        /* FLAG */ 1),
};
/* RSS uses final doorbell channel for SCP signalling */
struct mod_mhu3_channel_config scp_rss_channel_config[1] = {
    MOD_MHU3_INIT_DBCH(
        /* PBX CH */ SCP_MHU_SCP_RSS_NUM_DBCH - 1,
        /* FLAG */ 0,
        /* MBX CH */ SCP_MHU_SCP_RSS_NUM_DBCH - 1,
        /* FLAG */ 0),
};

static const struct fwk_element element_table[
    SCP_TC_MHU3_DEVICE_IDX_COUNT + 1] = {
    [SCP_TC_MHU3_DEVICE_IDX_SCP_AP_S] = {
        .name = "MHU_SCP_AP_S",
        .sub_element_count = 1,
        .data = &(struct mod_mhu3_device_config) {
            .irq = AP_SCP_MHU0_REC_IRQ,
            .in = SCP_MHU_SCP_AP_RCV_S,
            .out = SCP_MHU_SCP_AP_SND_S,
            .channels = ap_s_channels,
            .timer_id = FWK_ID_ELEMENT_INIT(FWK_MODULE_IDX_TIMER, 0),
            .resp_wait_timeout_us = (100 * 1000u),
        },
    },
    [SCP_TC_MHU3_DEVICE_IDX_SCP_AP_NS] = {
        .name = "MHU_SCP_AP_NS",
        .sub_element_count = 2,
        .data = &(struct mod_mhu3_device_config) {
            .irq = AP_SCP_MHU4_REC_IRQ,
            .in = SCP_MHU_SCP_AP_RCV_NS,
            .out = SCP_MHU_SCP_AP_SND_NS,
            .channels = ap_ns_channels,
            .timer_id = FWK_ID_ELEMENT_INIT(FWK_MODULE_IDX_TIMER, 0),
            .resp_wait_timeout_us = (100 * 1000u),
        },
    },
    [SCP_TC_MHU3_DEVICE_IDX_SCP_RSS_S] = {
        .name = "MHU_SCP_RSS_S",
        .sub_element_count = 1,
        .data = &(struct mod_mhu3_device_config) {
            .irq = RSS_SCP_MHU0_REC_IRQ,
            .in = SCP_MHU_SCP_RSS_RCV_S_CLUS0,
            .out = SCP_MHU_SCP_RSS_SND_S_CLUS0,
            .channels = scp_rss_channel_config,
            .timer_id = FWK_ID_ELEMENT_INIT(FWK_MODULE_IDX_TIMER, 0),
            .resp_wait_timeout_us = (100 * 1000u),
        },
    },
    [SCP_TC_MHU3_DEVICE_IDX_COUNT] = { 0 },
};

static const struct fwk_element *get_element_table(fwk_id_t module_id)
{
    return element_table;
}

const struct fwk_module_config config_mhu3 = {
    .elements = FWK_MODULE_DYNAMIC_ELEMENTS(get_element_table),
};
