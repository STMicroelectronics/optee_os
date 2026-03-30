/*
 * Arm SCP/MCP Software
 * Copyright (c) 2024-2025, Arm Limited and Contributors. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include "fvp_baser_aemv8r_mmap.h"
#include "mod_armv8r_mpu.h"

#include <fwk_macros.h>
#include <fwk_module.h>

#include <arch_reg.h>

#include <fmw_memory.h>

static uint8_t mem_attributes[] = {
    MAIR_NORMAL_WB_NT,
    MAIR_DEVICE_NGNRNE,
};

static struct mod_armv8r_mpu_region mem_regions[] = {
    { .prbar = PRBAR_VALUE(
          FMW_MEM0_BASE,
          PRBAR_SH_NON_SHAREABLE,
          PRBAR_AP_RO_EL2,
          PRBAR_XN_PERMITTED),
      .prlar = PRLAR_VALUE(
          FMW_MEM0_BASE + FMW_MEM0_SIZE - 1,
          PRLAR_NS_SECURE,
          MPU_ATTR_0,
          PRLAR_EN_ENABLED) },
    { .prbar = PRBAR_VALUE(
          FMW_MEM1_BASE,
          PRBAR_SH_NON_SHAREABLE,
          PRBAR_AP_RW_EL2,
          PRBAR_XN_NOT_PERMITTED),
      .prlar = PRLAR_VALUE(
          FMW_MEM1_BASE + FMW_MEM1_SIZE - 1,
          PRLAR_NS_SECURE,
          MPU_ATTR_0,
          PRLAR_EN_ENABLED) },
    { .prbar = PRBAR_VALUE(
          FVP_GIC_BASE,
          PRBAR_SH_NON_SHAREABLE,
          PRBAR_AP_RW_EL2,
          PRBAR_XN_NOT_PERMITTED),
      .prlar = PRLAR_VALUE(
          FVP_GIC_BASE + FVP_GIC_SIZE - 1,
          PRLAR_NS_SECURE,
          MPU_ATTR_1,
          PRLAR_EN_ENABLED) },
    { .prbar = PRBAR_VALUE(
          FVP_UART_BASE,
          PRBAR_SH_NON_SHAREABLE,
          PRBAR_AP_RW_EL2,
          PRBAR_XN_NOT_PERMITTED),
      .prlar = PRLAR_VALUE(
          FVP_UART_BASE + FVP_UART_SIZE - 1,
          PRLAR_NS_SECURE,
          MPU_ATTR_1,
          PRLAR_EN_ENABLED) },
    { .prbar = PRBAR_VALUE(
          FVP_GTIMER_BASE,
          PRBAR_SH_NON_SHAREABLE,
          PRBAR_AP_RW_EL2,
          PRBAR_XN_NOT_PERMITTED),
      .prlar = PRLAR_VALUE(
          FVP_GTIMER_BASE + FVP_GTIMER_SIZE - 1,
          PRLAR_NS_SECURE,
          MPU_ATTR_1,
          PRLAR_EN_ENABLED) },
    { .prbar = PRBAR_VALUE(
          FVP_CNTCONTROL_BASE,
          PRBAR_SH_NON_SHAREABLE,
          PRBAR_AP_RW_EL2,
          PRBAR_XN_NOT_PERMITTED),
      .prlar = PRLAR_VALUE(
          FVP_CNTCONTROL_BASE + FVP_CNTCONTROL_SIZE - 1,
          PRLAR_NS_SECURE,
          MPU_ATTR_1,
          PRLAR_EN_ENABLED) },
    { .prbar = PRBAR_VALUE(
          FVP_WATCHDOG_BASE,
          PRBAR_SH_NON_SHAREABLE,
          PRBAR_AP_RW_EL2,
          PRBAR_XN_NOT_PERMITTED),
      .prlar = PRLAR_VALUE(
          FVP_WATCHDOG_BASE + FVP_WATCHDOG_SIZE - 1,
          PRLAR_NS_SECURE,
          MPU_ATTR_1,
          PRLAR_EN_ENABLED) }
};

const struct fwk_module_config config_armv8r_mpu = {
    .data = &((struct mod_armv8r_mpu_config){
        .attributes_count = FWK_ARRAY_SIZE(mem_attributes),
        .attributes = mem_attributes,
        .region_count = FWK_ARRAY_SIZE(mem_regions),
        .regions = mem_regions,
    }),
};
