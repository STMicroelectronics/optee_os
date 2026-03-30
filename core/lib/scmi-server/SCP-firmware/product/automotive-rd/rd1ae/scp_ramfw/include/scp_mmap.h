/*
 * Arm SCP/MCP Software
 * Copyright (c) 2024, Arm Limited and Contributors. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 * Description:
 *     Base address definitions for the SCP's sub-system.
 */

#ifndef SCP_MMAP_H
#define SCP_MMAP_H

/* Base address and size of SCP's ITCM */
#define SCP_ITC_RAM_BASE (0x00000000)
#define SCP_ITC_RAM_SIZE (256 * 1024)

/* Base address and size of SCP's DTCM */
#define SCP_DTC_RAM_BASE (0x20000000)
#define SCP_DTC_RAM_SIZE (256 * 1024)

/* SCP sub-system peripherals */
#define SCP_AP2SCP_MHUV3_RCV_S_BASE   (0x45030000UL)
#define SCP_ATU_BASE                  (0x50010000UL)
#define SCP_POWER_CONTROL_BASE        (0x50000000UL)
#define SCP_PPU_SYS0_BASE             (0x50041000UL)
#define SCP_REFCLK_CNTCONTROL_BASE    (0x2A430000UL)
#define SCP_REFCLK_CNTCTL_BASE        (0x44000000UL)
#define SCP_REFCLK_CNTBASE0_BASE      (0x44001000UL)
#define SCP_RSE2SCP_MHUV3_RCV_S_BASE  (0x46010000UL)
#define SCP_SCP2AP_MHUV3_SEND_S_BASE  (0x45020000UL)
#define SCP_SCP2RSE_MHUV3_SEND_S_BASE (0x46000000UL)
#define SCP_SID_BASE                  (0x2A4A0000UL)
#define SCP_SYSTEM_PIK_BASE           (0x50040000UL)
#define SCP_UART_BASE                 (0x44002000UL)

/* Base address of SCP expansion memory regions */
#define SCP_SOC_EXPANSION3_BASE (0x40000000UL) /* 64MB size */

/* SCP addresses mapped via ATU into address translation windows */
#define SCP_ADDRESS_TRANSLATION_WINDOW0_BASE (0x60000000UL)
#define SCP_ADDRESS_TRANSLATION_WINDOW1_BASE (0xA0000000UL)

/*
 * Offsets within SCP's Address Translation Window0
 *          __________________________
 *         |                          |
 *         |     IO Block NCI 16M     |
 *         |__________________________| 0x78600000
 *         |                          |
 *         |     ATU MMIO (1M)        |
 *         |__________________________| 0x78500000
 *         |                          |
 *         |     RSM_SRAM 4M          |
 *         |__________________________| 0x78100000
 *         |                          |
 *         |     GPC_MMU 1M           |
 *         |__________________________| 0x78000000
 *         |                          |
 *         |     SHARED SRAM 128M     |
 *         |__________________________| 0x70000000
 *         |                          |
 *         |     CLUSTER UTIL 256M    |
 *         |__________________________| 0x60000000
 */

#define SCP_ATW0_CLUSTER_UTILITY_SIZE        (256 * FWK_MIB)
#define SCP_ATW0_AP_PERIPHERAL_SRAM_SIZE     (128 * FWK_MIB)
#define SCP_ATW0_AP_PERIPHERAL_GPC_SMMU_SIZE (1 * FWK_MIB)
#define SCP_ATW0_SHARED_SRAM_RSM_SIZE        (4 * FWK_MIB)
#define SCP_ATW0_ATU_MMIO_SIZE               (1 * FWK_MIB)
#define SCP_ATW0_AP_IO_BLOCK_NCI_SIZE        (16 * FWK_MIB)

#define SCP_ATW0_CLUSTER_UTILITY_BASE SCP_ADDRESS_TRANSLATION_WINDOW0_BASE
#define SCP_ATW0_AP_PERIPHERAL_SRAM_BASE \
    (SCP_ATW0_CLUSTER_UTILITY_BASE + SCP_ATW0_CLUSTER_UTILITY_SIZE)
#define SCP_ATW0_AP_PERIPHERAL_GPC_SMMU_BASE \
    (SCP_ATW0_AP_PERIPHERAL_SRAM_BASE + SCP_ATW0_AP_PERIPHERAL_SRAM_SIZE)
#define SCP_ATW0_SHARED_SRAM_RSM_BASE \
    (SCP_ATW0_AP_PERIPHERAL_GPC_SMMU_BASE + \
     SCP_ATW0_AP_PERIPHERAL_GPC_SMMU_SIZE)
#define SCP_ATW0_ATU_MMIO_BASE \
    (SCP_ATW0_SHARED_SRAM_RSM_BASE + SCP_ATW0_SHARED_SRAM_RSM_SIZE)
#define SCP_ATW0_AP_IO_BLOCK_NCI_BASE \
    SCP_ATW0_ATU_MMIO_BASE + SCP_ATW0_ATU_MMIO_SIZE

/*
 * Offsets within SCP's Address Translation Window1
 *          __________________________
 *         |                          |
 *         |         CMN 1G           |
 *         |__________________________| 0xA0000000
 */
#define SCP_ATW1_CMN_BASE (SCP_ADDRESS_TRANSLATION_WINDOW1_BASE)
#define SCP_ATW1_CMN_SIZE (1 * FWK_GIB)

/*
 * Size of SCP's view of per-cluster utility memory region.
 */
#define SCP_CLUSTER_UTILITY_SIZE (0x200000UL)

/*
 * Offsets of various blocks within cluster utility that is
 * mapped into SCP's address translation window 0. These offsets are applicable
 * to each cluster in the system.
 */
#define SCP_CLUSTER_UTILITY_CORE_MANAGER_OFFSET (0x80000UL)
#define SCP_CLUSTER_UTILITY_CLUSTER_PPU_OFFSET  (0x130000UL)
#define SCP_CLUSTER_UTILITY_CORE_PPU_OFFSET     (0x180000UL)

/* Core Manager base address for a cluster 'n' */
#define SCP_CLUSTER_UTILITY_CORE_MANAGER_BASE(n) \
    (SCP_ATW0_CLUSTER_UTILITY_BASE + (n * SCP_CLUSTER_UTILITY_SIZE) + \
     SCP_CLUSTER_UTILITY_CORE_MANAGER_OFFSET)

/* Cluster PPU base address */
#define SCP_CLUSTER_UTILITY_CLUSTER_PPU_BASE(n) \
    (SCP_ATW0_CLUSTER_UTILITY_BASE + (n * SCP_CLUSTER_UTILITY_SIZE) + \
     SCP_CLUSTER_UTILITY_CLUSTER_PPU_OFFSET)

/* Application core PPU base address */
#define SCP_CLUSTER_UTILITY_CORE_PPU_BASE(n) \
    (SCP_ATW0_CLUSTER_UTILITY_BASE + (n * SCP_CLUSTER_UTILITY_SIZE) + \
     SCP_CLUSTER_UTILITY_CORE_PPU_OFFSET)

/* CMN config space is mapped in the SCP address translation window 1 */
#define SCP_CMN_BASE SCP_ATW1_CMN_BASE

/*
 * Shared RSM SRAM (shared between RSE and SCP) is mapped by ATU in
 * the SCP address translation window 0 at the address 0x7810_0000.
 */
#define SCP_SHARED_SRAM_RSM_BASE (SCP_ATW0_SHARED_SRAM_RSM_BASE)

/* Base address of the IO block register bank. */
#define SCP_IO_BLOCK_BASE (0x280000000UL)
/* Base address of the NCI in the IO block. */
#define SCP_IO_BLOCK_NCI_GVP_BASE (SCP_IO_BLOCK_BASE + 0x4000000UL)
/* Size of the register bank of IO block. */
#define SCP_IO_BLOCK_REGISTER_SIZE (0x8000000UL)
/* Base address of th Control register set. */
#define SCP_IO_BLOCK_CONTROL_REG (SCP_IO_BLOCK_BASE + 0x5B00000UL)

#endif /* SCP_MMAP_H */
