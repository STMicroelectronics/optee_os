/*
 * Arm SCP/MCP Software
 * Copyright (c) 2024, Arm Limited and Contributors. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 * Description:
 *     Base address and offset definitions for the various addressable regions
 *     within the AP Peripheral memory region in the Cluster Utility memory
 *     map space.
 */

#ifndef SCP_CU_AP_PERIPH_H
#define SCP_CU_AP_PERIPH_H

#include "scp_atw0_mmap.h"

/*
 * Offsets of various blocks within a AP peripheral space (also referred to
 * as 'AP Access Group') in the cluster utility MMAP memory region. These
 * offsets are applicable to each cluster in the system.
 */

#define SCP_CU_AP_PERIPH_CORE_MANAGER_OFFSET (0x4000)
#define SCP_CU_AP_PERIPH_CLUSTER_PPU_OFFSET  (0x130000)
#define SCP_CU_AP_PERIPH_CORE_PPU_OFFSET     (0x180000)

/* Core Manager base address for a cluster 'n' */
#define SCP_CU_AP_PERIPH_CORE_MANAGER_BASE_N(n) \
    (SCP_CU_AP_PERIPH_BASE_N(n) + SCP_CU_AP_PERIPH_CORE_MANAGER_OFFSET)

/* Cluster PPU base address for a cluster 'n' */
#define SCP_CU_AP_PERIPH_CLUSTER_PPU_BASE(n) \
    (SCP_CU_AP_PERIPH_BASE_N(n) + SCP_CU_AP_PERIPH_CLUSTER_PPU_OFFSET)

/* Application core PPU base address for a core 'n' */
#define SCP_CU_AP_PERIPH_CORE_PPU_BASE(n) \
    (SCP_CU_AP_PERIPH_BASE_N(n) + SCP_CU_AP_PERIPH_CORE_PPU_OFFSET)

#endif /* SCP_CU_AP_PERIPH_H */
