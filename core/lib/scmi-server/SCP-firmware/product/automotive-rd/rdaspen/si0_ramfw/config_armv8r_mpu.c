/*
 * Arm SCP/MCP Software
 * Copyright (c) 2025, Arm Limited and Contributors. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include "mod_armv8r_mpu.h"
#include "si0_mmap.h"

#include <fwk_macros.h>
#include <fwk_module.h>

#include <arch_reg.h>

#include <fmw_memory.h>

static uint8_t mem_attributes[] = {
    MAIR_NORMAL_WB_NT,
    MAIR_DEVICE_NGNRNE,
    MAIR_NORMAL_NC,
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
          SI0_PERIPHERAL_BASE,
          PRBAR_SH_NON_SHAREABLE,
          PRBAR_AP_RW_EL2,
          PRBAR_XN_NOT_PERMITTED),
      .prlar = PRLAR_VALUE(
          SI0_PERIPHERAL_BASE + SI0_PERIPHERAL_SIZE - 1,
          PRLAR_NS_SECURE,
          MPU_ATTR_1,
          PRLAR_EN_ENABLED) },
    { .prbar = PRBAR_VALUE(
          SI0_GIC_BASE,
          PRBAR_SH_NON_SHAREABLE,
          PRBAR_AP_RW_EL2,
          PRBAR_XN_NOT_PERMITTED),
      .prlar = PRLAR_VALUE(
          SI0_GIC_BASE + SI0_GIC_SIZE - 1,
          PRLAR_NS_SECURE,
          MPU_ATTR_1,
          PRLAR_EN_ENABLED) },
    { .prbar = PRBAR_VALUE(
          SI0_ATW_IO_BASE,
          PRBAR_SH_OUTER_SHAREABLE,
          PRBAR_AP_RW_EL2,
          PRBAR_XN_NOT_PERMITTED),
      .prlar = PRLAR_VALUE(
          SI0_ATW_IO_BASE + SI0_ATW_IO_SIZE - 1,
          PRLAR_NS_SECURE,
          MPU_ATTR_1,
          PRLAR_EN_ENABLED) },
    { .prbar = PRBAR_VALUE(
          SI0_ATW_MEM_BASE,
          PRBAR_SH_OUTER_SHAREABLE,
          PRBAR_AP_RW_EL2,
          PRBAR_XN_NOT_PERMITTED),
      .prlar = PRLAR_VALUE(
          SI0_ATW_MEM_BASE + SI0_ATW_MEM_SIZE - 1,
          PRLAR_NS_SECURE,
          MPU_ATTR_0,
          PRLAR_EN_ENABLED) },
    { .prbar = PRBAR_VALUE(
          SI0_MHU_BASE,
          PRBAR_SH_OUTER_SHAREABLE,
          PRBAR_AP_RW_EL2,
          PRBAR_XN_NOT_PERMITTED),
      .prlar = PRLAR_VALUE(
          SI0_MHU_BASE + SI0_MHU_SIZE - 1,
          PRLAR_NS_SECURE,
          MPU_ATTR_1,
          PRLAR_EN_ENABLED) },
    { .prbar = PRBAR_VALUE(
          SI0_DEVICE_FMU_BASE,
          PRBAR_SH_OUTER_SHAREABLE,
          PRBAR_AP_RW_EL2,
          PRBAR_XN_NOT_PERMITTED),
      .prlar = PRLAR_VALUE(
          SI0_DEVICE_FMU_BASE + SI0_DEVICE_FMU_SIZE - 1,
          PRLAR_NS_SECURE,
          MPU_ATTR_1,
          PRLAR_EN_ENABLED) },
    { .prbar = PRBAR_VALUE(
          SI0_RSE_SHARED_SRAM_BASE,
          PRBAR_SH_OUTER_SHAREABLE,
          PRBAR_AP_RW_EL2,
          PRBAR_XN_NOT_PERMITTED),
      .prlar = PRLAR_VALUE(
          SI0_RSE_SHARED_SRAM_BASE + SI0_RSE_SHARED_SRAM_SIZE - 1,
          PRLAR_NS_SECURE,
          MPU_ATTR_2,
          PRLAR_EN_ENABLED) },
};

const struct fwk_module_config config_armv8r_mpu = {
    .data = &((struct mod_armv8r_mpu_config){
        .attributes_count = FWK_ARRAY_SIZE(mem_attributes),
        .attributes = mem_attributes,
        .region_count = FWK_ARRAY_SIZE(mem_regions),
        .regions = mem_regions,
    }),
};
