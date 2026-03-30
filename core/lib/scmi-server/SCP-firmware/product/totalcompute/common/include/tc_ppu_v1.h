/*
 * Arm SCP/MCP Software
 * Copyright (c) 2023-2024, Arm Limited and Contributors. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef TC_PPU_V1_H
#define TC_PPU_V1_H

#include <fwk_macros.h>

#include "scp_mmap.h"
#include "config_power_domain.h"
#include "tc_core.h"

#define CORE_PPU_ELEMENT_INIT(_core_num, _cluster_idx) \
    { \
        .name = "CORE" FWK_STRINGIFY(_core_num), \
        .data = &((struct mod_ppu_v1_pd_config){ \
            .pd_type = MOD_PD_TYPE_CORE, \
            .ppu.reg_base = SCP_PPU_CORE_BASE(_core_num), \
            .ppu.irq = FWK_INTERRUPT_NONE, \
            .cluster_id = FWK_ID_ELEMENT_INIT(FWK_MODULE_IDX_PPU_V1, \
                    _cluster_idx), \
            .observer_id = FWK_ID_NONE_INIT, \
        }), \
    }

#define CORE_PPU_ELEMENT_INIT_WITH_IDX(_core_num, _cluster_idx) \
    [PPU_V1_ELEMENT_IDX_CORE##_core_num] = \
    CORE_PPU_ELEMENT_INIT(_core_num, _cluster_idx)

#define CLUSTER_PPU_ELEMENT_INIT(_cluster_num) \
    { \
        .name = "CLUSTER" FWK_STRINGIFY(_cluster_num), \
        .data = &((struct mod_ppu_v1_pd_config){ \
            .pd_type = MOD_PD_TYPE_CLUSTER, \
            .ppu.reg_base = SCP_PPU_CLUSTER_BASE, \
            .ppu.irq = FWK_INTERRUPT_NONE, \
            .observer_id = FWK_ID_NONE_INIT, \
        }), \
    }

#define CLUSTER_PPU_ELEMENT_INIT_WITH_IDX(_cluster_num) \
    [PPU_V1_ELEMENT_IDX_CLUSTER##_cluster_num] = \
    CLUSTER_PPU_ELEMENT_INIT(_cluster_num)

#define PPU_V1_ENUM_CORE_GEN(_core_num) \
    PPU_V1_ELEMENT_IDX_CORE##_core_num

#define PPU_V1_ENUM_CLUSTER_GEN(_cluster_num) \
    PPU_V1_ELEMENT_IDX_CLUSTER##_cluster_num

/* Elements in the PPU_V1 module. One for each:
 * - Core
 * - Cluster
 */
enum ppu_v1_dev_idx {
    TC_FOR_EACH_CORE(PPU_V1_ENUM_CORE_GEN),
    TC_FOR_EACH_CLUSTER(PPU_V1_ENUM_CLUSTER_GEN),
    PPU_V1_ELEMENT_IDX_MAX,
};

#undef PPU_V1_ENUM_CORE_GEN
#undef PPU_V1_ENUM_CLUSTER_GEN

#endif /* TC_PPU_V1_H */
