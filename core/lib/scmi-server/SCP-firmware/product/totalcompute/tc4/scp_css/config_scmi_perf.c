/*
 * Arm SCP/MCP Software
 * Copyright (c) 2023-2024, Arm Limited and Contributors. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include "tc4_dvfs.h"
#include "tc4_scmi.h"
#include "tc4_timer.h"

#include <scp_mmap.h>

#include <mod_scmi_perf.h>
#include <mod_transport.h>

#include <fwk_module.h>
#include <fwk_module_idx.h>

#include <stdint.h>

static const struct mod_scmi_perf_domain_config
    domains[DVFS_ELEMENT_IDX_COUNT] = {
        [DVFS_ELEMENT_IDX_GROUP_LITTLE] = {
#ifdef BUILD_HAS_SCMI_PERF_FAST_CHANNELS
         .fch_config = (struct scmi_perf_fch_config[]) {
            [MOD_SCMI_PERF_FAST_CHANNEL_LEVEL_SET] =
                FCH_INIT(TC4_TRANSPORT_SCMI_PERF_FCH_GROUP_LITTLE_LEVEL_SET),
            [MOD_SCMI_PERF_FAST_CHANNEL_LIMIT_SET] =
                FCH_INIT(TC4_TRANSPORT_SCMI_PERF_FCH_GROUP_LITTLE_LIMIT_SET),
            [MOD_SCMI_PERF_FAST_CHANNEL_LEVEL_GET] =
                FCH_INIT(TC4_TRANSPORT_SCMI_PERF_FCH_GROUP_LITTLE_LEVEL_GET),
            [MOD_SCMI_PERF_FAST_CHANNEL_LIMIT_GET] =
                FCH_INIT(TC4_TRANSPORT_SCMI_PERF_FCH_GROUP_LITTLE_LIMIT_GET),
        },
        .supports_fast_channels = true,
#endif
        },
        [DVFS_ELEMENT_IDX_GROUP_MID] = {
#ifdef BUILD_HAS_SCMI_PERF_FAST_CHANNELS
         .fch_config = (struct scmi_perf_fch_config[]) {
            [MOD_SCMI_PERF_FAST_CHANNEL_LEVEL_SET] =
                FCH_INIT(TC4_TRANSPORT_SCMI_PERF_FCH_GROUP_MID_LEVEL_SET),
            [MOD_SCMI_PERF_FAST_CHANNEL_LIMIT_SET] =
                FCH_INIT(TC4_TRANSPORT_SCMI_PERF_FCH_GROUP_MID_LIMIT_SET),
            [MOD_SCMI_PERF_FAST_CHANNEL_LEVEL_GET] =
                FCH_INIT(TC4_TRANSPORT_SCMI_PERF_FCH_GROUP_MID_LEVEL_GET),
            [MOD_SCMI_PERF_FAST_CHANNEL_LIMIT_GET] =
                FCH_INIT(TC4_TRANSPORT_SCMI_PERF_FCH_GROUP_MID_LIMIT_GET),
        },
        .supports_fast_channels = true,
#endif
        },
        [DVFS_ELEMENT_IDX_GROUP_BIG] = {
#ifdef BUILD_HAS_SCMI_PERF_FAST_CHANNELS
         .fch_config = (struct scmi_perf_fch_config[]) {
            [MOD_SCMI_PERF_FAST_CHANNEL_LEVEL_SET] =
                FCH_INIT(TC4_TRANSPORT_SCMI_PERF_FCH_GROUP_BIG_LEVEL_SET),
            [MOD_SCMI_PERF_FAST_CHANNEL_LIMIT_SET] =
                FCH_INIT(TC4_TRANSPORT_SCMI_PERF_FCH_GROUP_BIG_LIMIT_SET),
            [MOD_SCMI_PERF_FAST_CHANNEL_LEVEL_GET] =
                FCH_INIT(TC4_TRANSPORT_SCMI_PERF_FCH_GROUP_BIG_LEVEL_GET),
            [MOD_SCMI_PERF_FAST_CHANNEL_LIMIT_GET] =
                FCH_INIT(TC4_TRANSPORT_SCMI_PERF_FCH_GROUP_BIG_LIMIT_GET),
        },
        .supports_fast_channels = true,
#endif
        },
        [DVFS_ELEMENT_IDX_GPU] = {
#ifdef BUILD_HAS_SCMI_PERF_FAST_CHANNELS
         .fch_config = (struct scmi_perf_fch_config[]) {
            [MOD_SCMI_PERF_FAST_CHANNEL_LEVEL_SET] =
                FCH_INIT(TC4_TRANSPORT_SCMI_PERF_FCH_GPU_LEVEL_SET),
            [MOD_SCMI_PERF_FAST_CHANNEL_LIMIT_SET] =
                FCH_INIT(TC4_TRANSPORT_SCMI_PERF_FCH_GPU_LIMIT_SET),
            [MOD_SCMI_PERF_FAST_CHANNEL_LEVEL_GET] =
                FCH_INIT(TC4_TRANSPORT_SCMI_PERF_FCH_GPU_LEVEL_GET),
            [MOD_SCMI_PERF_FAST_CHANNEL_LIMIT_GET] =
                FCH_INIT(TC4_TRANSPORT_SCMI_PERF_FCH_GPU_LIMIT_GET),
        },
        .supports_fast_channels = true,
#endif
        },
    };

#if (PLATFORM_VARIANT == TC4_VAR_EXPERIMENT_POWER)
static const struct mod_scmi_plugin_config plugins_table[] = {
    [0] = {
        .id = FWK_ID_MODULE_INIT(FWK_MODULE_IDX_TRAFFIC_COP),
        .dom_type = PERF_PLUGIN_DOM_TYPE_PHYSICAL,
    },
    [1] = {
        .id = FWK_ID_MODULE_INIT(FWK_MODULE_IDX_MPMM),
        .dom_type = PERF_PLUGIN_DOM_TYPE_PHYSICAL,
    },
    [2] = {
        .id = FWK_ID_MODULE_INIT(FWK_MODULE_IDX_THERMAL_MGMT),
        .dom_type = PERF_PLUGIN_DOM_TYPE_FULL,
    },
};
#elif (PLATFORM_VARIANT == TC4_VAR_MPMM)
static const struct mod_scmi_plugin_config plugins_table[] = {
    [0] = {
        .id = FWK_ID_MODULE_INIT(FWK_MODULE_IDX_MPMM),
        .dom_type = PERF_PLUGIN_DOM_TYPE_PHYSICAL,
    },
};
#else
static const struct mod_scmi_plugin_config plugins_table[] = {};
#endif

const struct fwk_module_config config_scmi_perf = {
    .data = &((struct mod_scmi_perf_config){
        .domains = &domains,
        .perf_doms_count = FWK_ARRAY_SIZE(domains),
#ifdef BUILD_HAS_SCMI_PERF_FAST_CHANNELS
        .fast_channels_rate_limit = (4 * 1000),
#endif
        .plugins = plugins_table,
        .plugins_count = FWK_ARRAY_SIZE(plugins_table),
    }),
};
