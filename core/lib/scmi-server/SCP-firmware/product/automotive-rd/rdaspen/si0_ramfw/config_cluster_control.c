/*
 * Arm SCP/MCP Software
 * Copyright (c) 2025, Arm Limited and Contributors. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 * Description:
 *     Configuration data for module 'cluster_control'.
 */

#include "platform_core.h"
#include "si0_mmap.h"

#include <mod_cluster_control.h>
#include <mod_si0_platform.h>

#include <fwk_id.h>
#include <fwk_module.h>
#include <fwk_module_idx.h>

#define RDASPEN_AP_RVBAR             0x82000
#define CLUSTER_CONTROL_REGION_COUNT NUMBER_OF_CLUSTERS

#define CLUSTER_CONTROL_REGION_ADDR(cluster_idx) \
    SI0_ATW1_CLUSTER_UTILITY_BASE + (cluster_idx * SI0_CLUSTER_UTILITY_SIZE) + \
        SI0_CLUSTER_UTILITY_CLUSTER_CONTROL_OFFSET

static const uintptr_t cluster_control_regions[CLUSTER_CONTROL_REGION_COUNT] = {
    CLUSTER_CONTROL_REGION_ADDR(0),
    CLUSTER_CONTROL_REGION_ADDR(1),
    CLUSTER_CONTROL_REGION_ADDR(2),
    CLUSTER_CONTROL_REGION_ADDR(3),
};

static const struct mod_cluster_control_config cluster_control_config = {
    .regions = cluster_control_regions,
    .region_count = CLUSTER_CONTROL_REGION_COUNT,
    .rvbar = RDASPEN_AP_RVBAR,
    .platform_notification = {
        .notification_id = FWK_ID_NOTIFICATION_INIT(
            FWK_MODULE_IDX_SI0_PLATFORM,
            MOD_SI0_PLATFORM_NOTIFICATION_IDX_SUBSYS_INITIALIZED),
        .source_id = FWK_ID_MODULE_INIT(
            FWK_MODULE_IDX_SI0_PLATFORM),
    },
};

struct fwk_module_config config_cluster_control = {
    .data = &cluster_control_config,
};
