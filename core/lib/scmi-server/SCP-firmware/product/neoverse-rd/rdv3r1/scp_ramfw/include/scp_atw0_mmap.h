/*
 * Arm SCP/MCP Software
 * Copyright (c) 2025, Arm Limited and Contributors. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 * Description:
 *     Base address definitions for the carveouts in SCP's Address Translation
 *     Window 0 (ATW0).
 */

#ifndef SCP_ATW0_MMAP_H
#define SCP_ATW0_MMAP_H

#include "platform_core.h"
#include "scp_css_mmap.h"

// clang-format off

/*
 * Offsets within SCP's Address Translation Window 0.
 *
 *   +----------------------------------------+ SCP_ADDRESS_TRANSLATION_WINDOW0_END
 *   |                                        | [SCP_ADDRESS_TRANSLATION_WINDOW0_BASE + 1GB]
 *   |                                        |
 *   ~ Unmapped Space                         ~
 *   |                                        |
 *   |                                        |
 *   +----------------------------------------+
 *   |                                        |
 *   | MMIO region mapped by ATU (1MB)        |
 *   |                                        |
 *   +----------------------------------------+ SCP_ATW0_ATU_MMIO_BASE
 *   |                                        |
 *   | Peripheral SRAM (1MB)                  |
 *   |                                        |
 *   +----------------------------------------+ SCP_ATW0_AP_PERIPHERAL_SRAM_BASE
 *   |                                        |
 *   | Cluster Utility LCP Peripherals        |
 *   | (LCP Group)                            |
 *   |                                        |
 *   +----------------------------------------+ SCP_ATW0_CU_LCP_PERIPH_REGION_BASE
 *   |                                        |
 *   | Cluster Utility AP Peripherals         |
 *   | (AP Access Group)                      |
 *   |                                        |
 *   +----------------------------------------+ SCP_ATW0_CU_AP_PERIPH_REGION_BASE
 *                                              [SCP_ADDRESS_TRANSLATION_WINDOW0_BASE]
 *
 */

// clang-format on

/*
 * Definitions related to AP Peripheral Region (also referred to as 'AP Access
 * Group') in the cluster utility (CU) MMAP address space.
 *
 * For each cluster on the platform, there is a AP Peripheral Group (or AP
 * Access Group) of size SCP_CU_AP_PERIPH_SIZE. All the AP peripheral group
 * instances are sequentially placed starting from
 * SCP_ATW0_CU_AP_PERIPH_REGION_BASE and the total size of this region is
 * SCP_ATW0_CU_AP_PERIPH_REGION_SIZE. The base address of a AP peripheral group
 * can be determined using SCP_CU_AP_PERIPH_BASE_N.
 *
 * ATU is used to map the entire AP peripheral region with the logical base
 * address of SCP_ATW0_CU_AP_PERIPH_REGION_BASE and size
 * SCP_ATW0_CU_AP_PERIPH_REGION_SIZE. This region is mapped to physical address
 * of 0x140000000 in the AP memory map.
 *
 */
#define SCP_CU_AP_PERIPH_SIZE             (0x200000)
#define SCP_ATW0_CU_AP_PERIPH_REGION_BASE (SCP_ADDRESS_TRANSLATION_WINDOW0_BASE)
#define SCP_ATW0_CU_AP_PERIPH_REGION_SIZE \
    (MAX_NUM_CLUSTERS * SCP_CU_AP_PERIPH_SIZE)
#define SCP_CU_AP_PERIPH_BASE_N(n) \
    (SCP_ATW0_CU_AP_PERIPH_REGION_BASE + (n * SCP_CU_AP_PERIPH_SIZE))

/*
 * Definitions related to LCP Peripheral Region (also referred to as 'LCP
 * Group') in the cluster utility (CU) MMAP address space.
 *
 * For each cluster on the platform, there is a LCP peripheral (or LCP Group) of
 * size SCP_CU_LCP_PERIPH_SIZE. All the LCP peripheral instances are
 * sequentially placed starting from SCP_ATW0_CU_LCP_PERIPH_REGION_BASE and the
 * total size of this region is SCP_ATW0_CU_LCP_PERIPH_REGION_SIZE. The base
 * address of a LCP peripheral can be determined using SCP_CU_LCP_PERIPH_BASE_N.
 *
 * ATU is used to map the entire LCP peripheral region with the logical base
 * address of SCP_ATW0_CU_LCP_PERIPH_REGION_BASE and size
 * SCP_ATW0_CU_LCP_PERIPH_REGION_SIZE. This region is mapped to physical address
 * of 0x160000000 in the AP memory map.
 *
 */
#define SCP_CU_LCP_PERIPH_SIZE (0x200000)
#define SCP_ATW0_CU_LCP_PERIPH_REGION_BASE \
    (SCP_ATW0_CU_AP_PERIPH_REGION_BASE + SCP_ATW0_CU_AP_PERIPH_REGION_SIZE)
#define SCP_ATW0_CU_LCP_PERIPH_REGION_SIZE \
    (MAX_NUM_LCP * SCP_CU_LCP_PERIPH_SIZE)
#define SCP_CU_LCP_PERIPH_BASE_N(n) \
    (SCP_ATW0_CU_LCP_PERIPH_REGION_BASE + (n * SCP_CU_LCP_PERIPH_SIZE))

/* Logical address to access AP shared SRAM via ATU */
#define SCP_ATW0_AP_PERIPHERAL_SRAM_BASE \
    (SCP_ATW0_CU_LCP_PERIPH_REGION_BASE + SCP_ATW0_CU_LCP_PERIPH_REGION_SIZE)
#define SCP_ATW0_AP_PERIPHERAL_SRAM_SIZE (1 * FWK_MIB)

/* Logical address to access MMIO region mapped by ATU */
#define SCP_ATW0_ATU_MMIO_BASE \
    (SCP_ATW0_AP_PERIPHERAL_SRAM_BASE + SCP_ATW0_AP_PERIPHERAL_SRAM_SIZE)
#define SCP_ATW0_ATU_MMIO_SIZE (1 * FWK_MIB)

#endif /* SCP_ATW0_MMAP_H */
