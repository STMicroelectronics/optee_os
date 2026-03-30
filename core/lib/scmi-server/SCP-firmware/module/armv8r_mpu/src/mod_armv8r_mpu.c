/*
 * Arm SCP/MCP Software
 * Copyright (c) 2024, Arm Limited and Contributors. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include "internal/armv8r_mpu_reg.h"
#include "mod_armv8r_mpu.h"

#include <fwk_assert.h>
#include <fwk_id.h>
#include <fwk_module.h>
#include <fwk_status.h>

#define SCTLR_EL2_M      (1 << 0)
#define MPUIR_EL2_REGION (0xFF)

static void set_mem_attr(uint8_t attr_index, uint8_t value)
{
    uint64_t reg_value = read_mair_el2();
    reg_value |= (value << (attr_index * 8));
    write_mair_el2(reg_value);
    barrier_dsync_fence_full();
}

static void set_mem_region(uint8_t region_index, uint64_t prbar, uint64_t prlar)
{
    write_prselr_el2(region_index);
    barrier_isync_fence_full();
    write_prbar_el2(prbar);
    write_prlar_el2(prlar);
    barrier_dsync_fence_full();
}

static int armv8r_mpu_init(
    fwk_id_t module_id,
    unsigned int element_count,
    const void *data)
{
    const struct mod_armv8r_mpu_config *config;
    uint8_t index;
    uint64_t max_regions;

    fwk_assert(element_count == 0);
    fwk_assert(data != NULL);

    config = data;

    if (config->attributes_count >= (uint8_t)MPU_MAX_ATTR_COUNT) {
        return FWK_E_RANGE;
    }

    max_regions = (read_mpuir_el2() & MPUIR_EL2_REGION);
    if (config->region_count >= max_regions) {
        return FWK_E_RANGE;
    }

    /* Disable MPU */
    write_sctlr_el2(read_sctlr_el2() & ~SCTLR_EL2_M);
    barrier_dsync_fence_full();
    barrier_isync_fence_full();

    /* Write memory attributes */
    for (index = 0U; index < config->attributes_count; index++) {
        set_mem_attr(index, config->attributes[index]);
    }

    /* Write memory regions */
    for (index = 0U; index < config->region_count; index++) {
        set_mem_region(
            index, config->regions[index].prbar, config->regions[index].prlar);
    }

    /* Clear unused memory regions */
    for (index = config->region_count; index < max_regions; index++) {
        set_mem_region(index, 0, 0);
    }

    /* Enable MPU */
    write_sctlr_el2(read_sctlr_el2() | SCTLR_EL2_M);
    barrier_dsync_fence_full();
    barrier_isync_fence_full();

    return FWK_SUCCESS;
}

/* Module description */
const struct fwk_module module_armv8r_mpu = {
    .type = FWK_MODULE_TYPE_DRIVER,
    .init = armv8r_mpu_init,
};
