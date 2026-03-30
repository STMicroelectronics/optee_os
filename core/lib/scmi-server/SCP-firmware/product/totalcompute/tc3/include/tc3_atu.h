/*
 * Arm SCP/MCP Software
 * Copyright (c) 2023-2024, Arm Limited and Contributors. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef TC3_ATU_H
#define TC3_ATU_H

/*
 * There are different system access ports through ATU.
 * It is up to the platform to choose various ATU regions
 * from these available ports.
 * However, memory attributes have to be considered.
 * For example, CLUSTER UTILITY space requires the region to
 * be defined in DEVICE type.Hence we chose one of the available
 * regions(Integ expansion) from DEVICE type.
 *
 *  The below diagram summarizes the ATU mapping in the system.
 *
 *======================================================
 *                  DATA0 NS PORT
 *======================================================
 *  0x6000_0000  ------------------   SCP_ATU_LOG_ADDR_SHARED_NSRAM
 *               |  SHARED NSRAM  |
 *               |     (512KB)    |
 *  0x6008_0000  ------------------
 *======================================================
 *                  DATA0 S PORT
 *======================================================
 *  0x7000_0000  ------------------   SCP_ATU_LOG_ADDR_REFCLK
 *               |     REFCLK     |
 *               |     (128K)     |
 *  0x7002_0000  ------------------   SCP_ATU_LOG_ADDR_REFCLK_AP
 *               |    REFCLK_AP   |
 *               |     (128K)     |
 *  0x7004_0000  ------------------   SCP_ATU_LOG_ADDR_SHARED_SRAM
 *               |   SHARED SRAM  |
 *               |     (512KB)    |
 *  0x700C_0000  ------------------   SCP_ATU_LOG_ADDR_ROS_SOC_BASE
 *               |  ROS SOC BASE  |
 *               |     (64KB)     |
 *  0x700E_0000  ------------------
 *======================================================
 *             INTEGRATION EXP S PORT
 *======================================================
 *  0xB000_0000  ------------------   SCP_ATU_LOG_ADDR_CLUSTER_UTIL
 *               |     CLUS UTIL  |
 *               |      (8MB)     |
 *  0xB080_0000  ------------------
 *======================================================
 *                  DEVICE S PORT
 *======================================================
 *  0xD000_0000  ------------------   SCP_ATU_LOG_ADDR_PLL
 *               |       PLL      |
 *               |      (64KB)    |
 *  0xD001_0000  ------------------
 *
 *
 */

/* ATU System Acccess Ports */
#define SCP_SYSTEM_ACCESS_ATU_DATA0_BASE_NS   UINT32_C(0x60000000)
#define SCP_SYSTEM_ACCESS_ATU_DATA0_BASE_S    UINT32_C(0x70000000)
#define SCP_SYSTEM_ACCESS_ATU_DATA1_BASE_NS   UINT32_C(0x80000000)
#define SCP_SYSTEM_ACCESS_ATU_DATA1_BASE_S    UINT32_C(0x90000000)
#define SCP_SYSTEM_ACCESS_ATU_INT_EXP_BASE_NS UINT32_C(0xA0000000)
#define SCP_SYSTEM_ACCESS_ATU_INT_EXP_BASE_S  UINT32_C(0xB0000000)
#define SCP_SYSTEM_ACCESS_ATU_DEV_BASE_NS     UINT32_C(0xC0000000)
#define SCP_SYSTEM_ACCESS_ATU_DEV_BASE_S      UINT32_C(0xD0000000)

/* ATU Logical Addresses */
/* Data 0 Non-Secure port */
#define SCP_ATU_LOG_ADDR_SHARED_NSRAM (SCP_SYSTEM_ACCESS_ATU_DATA0_BASE_NS)
/* Data 0 Secure port */
#define SCP_ATU_LOG_ADDR_REFCLK (SCP_SYSTEM_ACCESS_ATU_DATA0_BASE_S)
#define SCP_ATU_LOG_ADDR_REFCLK_AP \
    (SCP_ATU_LOG_ADDR_REFCLK + SCP_ATU_REG_SIZE_REFCLK)
#define SCP_ATU_LOG_ADDR_SHARED_SRAM \
    (SCP_ATU_LOG_ADDR_REFCLK_AP + SCP_ATU_REG_SIZE_REFCLK_AP)
#define SCP_ATU_LOG_ADDR_ROS_SOC_BASE \
    (SCP_ATU_LOG_ADDR_SHARED_SRAM + SCP_ATU_REG_SIZE_SHARED_SRAM)
/* Integration Expansion Secure port */
#define SCP_ATU_LOG_ADDR_CLUSTER_UTIL (SCP_SYSTEM_ACCESS_ATU_INT_EXP_BASE_S)
#define SCP_ATU_LOG_ADDR_UART \
    (SCP_ATU_LOG_ADDR_CLUSTER_UTIL + SCP_ATU_REG_SIZE_CLUSTER_UTIL)
/* Device Secure port */
#define SCP_ATU_LOG_ADDR_PLL (SCP_SYSTEM_ACCESS_ATU_DEV_BASE_S)

/* ATU Physical Addresses */
#define SCP_ATU_PHY_ADDR_REFCLK       (0x2A810000)
#define SCP_ATU_PHY_ADDR_REFCLK_AP    (0x2A430000)
#define SCP_ATU_PHY_ADDR_CLUSTER_UTIL (0x5F000000)
#define SCP_ATU_PHY_ADDR_UART         (0x7FF70000) /* SoC UART0 */
#define SCP_ATU_PHY_ADDR_SHARED_SRAM  (0x04000000)
#define SCP_ATU_PHY_ADDR_SHARED_NSRAM (0x06000000)
#define SCP_ATU_PHY_ADDR_ROS_SOC      (0x7FF70000)
#define SCP_ATU_PHY_ADDR_PLL          (0x43000000)

/* ATU Region Sizes */
#define SCP_ATU_REG_SIZE_REFCLK       (128 * FWK_KIB)
#define SCP_ATU_REG_SIZE_REFCLK_AP    (128 * FWK_KIB)
#define SCP_ATU_REG_SIZE_UART         (128 * FWK_KIB)
#define SCP_ATU_REG_SIZE_SHARED_SRAM  (512 * FWK_KIB)
#define SCP_ATU_REG_SIZE_SHARED_NSRAM (512 * FWK_KIB)
#define SCP_ATU_REG_SIZE_ROS_SOC      (64 * FWK_KIB)
#define SCP_ATU_REG_SIZE_CLUSTER_UTIL (8 * FWK_MIB)
#define SCP_ATU_REG_SIZE_PLL          (64 * FWK_KIB)

#endif /* TC3_ATU_H */
