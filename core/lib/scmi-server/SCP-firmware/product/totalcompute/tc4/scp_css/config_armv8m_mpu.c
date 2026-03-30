/*
 * Arm SCP/MCP Software
 * Copyright (c) 2023-2024, Arm Limited and Contributors. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include "mod_armv8m_mpu.h"
#include "scp_mmap.h"

#include <fwk_macros.h>
#include <fwk_module.h>

#include <fmw_cmsis.h>

static const uint8_t attributes[] = {
    /* Device memory, non Gathering, non Re-ordering, non Early Write
       Acknowledgement */
    [MPU_ATTR_0] =
        ARM_MPU_ATTR(ARM_MPU_ATTR_DEVICE, ARM_MPU_ATTR_DEVICE_nGnRnE),
    /* Normal memory, non Cacheable */
    [MPU_ATTR_1] =
        ARM_MPU_ATTR(ARM_MPU_ATTR_NON_CACHEABLE, ARM_MPU_ATTR_NON_CACHEABLE),
};

static const ARM_MPU_Region_t regions[] = {
    {
        /* SCP_CORE_TCM_REGION*/
        .RBAR = ARM_MPU_RBAR(
            SCP_CORE_ITCM_REGION_START, /* BASE */
            ARM_MPU_SH_NON, /* SH */
            0, /* RO */
            0, /* NP */
            0 /* XN */),
        .RLAR = ARM_MPU_RLAR(SCP_CORE_ITCM_REGION_END, MPU_ATTR_1),
    },
    {
        /* SCP_CORE_DTCM_REGION*/
        .RBAR = ARM_MPU_RBAR(
            SCP_CORE_DTCM_REGION_START, /* BASE */
            ARM_MPU_SH_INNER, /* SH */
            0, /* RO */
            0, /* NP */
            1 /* XN */),
        .RLAR = ARM_MPU_RLAR(SCP_CORE_DTCM_REGION_END, MPU_ATTR_1),
    },
    {
        /* SCP_CORE_PERIPHERAL_REGION */
        .RBAR = ARM_MPU_RBAR(
            SCP_CORE_PERIPHERAL_REGION_START, /* BASE */
            ARM_MPU_SH_NON, /* SH */
            0, /* RO */
            0, /* NP */
            1 /* XN */),
        .RLAR = ARM_MPU_RLAR(SCP_CORE_PERIPHERAL_REGION_END, MPU_ATTR_0),
    },
    {
        /* SCP_SRAM_REGION*/
        .RBAR = ARM_MPU_RBAR(
            SCP_SRAM_REGION_START, /* BASE */
            ARM_MPU_SH_OUTER, /* SH */
            0, /* RO */
            0, /* NP */
            1 /* XN */),
        .RLAR = ARM_MPU_RLAR(SCP_SRAM_REGION_END, MPU_ATTR_1),
    },
    {
        /* SCP_DEVICE_REGION */
        .RBAR = ARM_MPU_RBAR(
            SCP_DEVICE_REGION_START, /* BASE */
            ARM_MPU_SH_OUTER, /* SH */
            0, /* RO */
            0, /* NP */
            1 /* XN */),
        .RLAR = ARM_MPU_RLAR(SCP_DEVICE_REGION_END, MPU_ATTR_0),
    },
};

const struct fwk_module_config config_armv8m_mpu = {
    .data = &((struct mod_armv8m_mpu_config){
        .region_count = FWK_ARRAY_SIZE(regions),
        .regions = regions,
        .attributes_count = FWK_ARRAY_SIZE(attributes),
        .attributes = attributes,
    }),
};
