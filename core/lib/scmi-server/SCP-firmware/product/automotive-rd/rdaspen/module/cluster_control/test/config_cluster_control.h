/*
 * Arm SCP/MCP Software
 * Copyright (c) 2025, Arm Limited and Contributors. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 * Description:
 *     Configuration data for module 'cluster_control'.
 */

#include <mod_cluster_control.h>

#include <fwk_element.h>
#include <fwk_id.h>
#include <fwk_macros.h>
#include <fwk_module.h>
#include <fwk_module_idx.h>

static uint8_t cluster_control_reg[2][64 * FWK_KIB] = { 0 };

static const uintptr_t cluster_control_regions[2] = {
    (uintptr_t)cluster_control_reg[0],
    (uintptr_t)cluster_control_reg[1],
};

static struct mod_cluster_control_config cluster_control_config_direct = {
    .regions = cluster_control_regions,
    .region_count = 2,
    .rvbar = 0xCDCDCDCDABABABAB,
};

static struct mod_cluster_control_config
    cluster_control_config_notification = { .regions = cluster_control_regions,
                                            .region_count = 2,
                                            .rvbar = 0xCDCDCDCDABABABAB,
                                            .platform_notification = {
                                                .notification_id =
                                                    test_module_notification_test,
                                                .source_id =
                                                    fwk_module_id_test_module,
                                            } };
