/*
 * Arm SCP/MCP Software
 * Copyright (c) 2024, Arm Limited and Contributors. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef TC4_PPU_V1_H
#define TC4_PPU_V1_H

#include "config_power_domain.h"
#include "scp_mmap.h"
#include "tc4_core.h"

/* TC4 contains two SYSTOP power domains:
 * - SYSTOP0: This manages all system logic
 * except the MCN slices
 * - SYSTOP1: This manages the MCN slices
 */
#define PPU_V1_FOR_EACH_SYSTOP(_func) _func(0), _func(1)

#define PPU_V1_ENUM_CORE_GEN(_core_num) PPU_V1_ELEMENT_IDX_CORE##_core_num

#define PPU_V1_ENUM_CLUSTER_GEN(_cluster_num) \
    PPU_V1_ELEMENT_IDX_CLUSTER##_cluster_num

#define PPU_V1_ENUM_CME_GEN(_cme_num) PPU_V1_ELEMENT_IDX_CME##_cme_num

#define PPU_V1_ENUM_SYSTOP_GEN(_systop_num) \
    PPU_V1_ELEMENT_IDX_SYSTOP##_systop_num

/* Elements in the PPU_V1 module. One for each:
 * - Core
 * - Cluster
 * - CME
 * - SYSTOP power domain
 */
enum ppu_v1_dev_idx {
    TC4_FOR_EACH_CORE(PPU_V1_ENUM_CORE_GEN),
    TC4_FOR_EACH_CLUSTER(PPU_V1_ENUM_CLUSTER_GEN),
#if !defined(PLAT_FVP)
    TC4_FOR_EACH_CME(PPU_V1_ENUM_CME_GEN),
#endif
    PPU_V1_FOR_EACH_SYSTOP(PPU_V1_ENUM_SYSTOP_GEN),
    PPU_V1_ELEMENT_IDX_MAX
};

#undef PPU_V1_ENUM_CORE_GEN
#undef PPU_V1_ENUM_CLUSTER_GEN

#endif /* TC4_PPU_V1_H */
