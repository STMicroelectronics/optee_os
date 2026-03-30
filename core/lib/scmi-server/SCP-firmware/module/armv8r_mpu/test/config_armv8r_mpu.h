/*
 * Arm SCP/MCP Software
 * Copyright (c) 2024, Arm Limited and Contributors. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <mod_armv8r_mpu.h>

#include <fwk_element.h>
#include <fwk_id.h>
#include <fwk_macros.h>
#include <fwk_module.h>

static uint8_t mem_attributes[] = {
    0xFF,
    0x00,
};

static struct mod_armv8r_mpu_region mem_regions[] = { { .prbar = 0,
                                                        .prlar = 0x7fffffC1 },
                                                      {
                                                          .prbar = 0x80000000,
                                                          .prlar = 0xffffffC3,
                                                      } };

const struct fwk_module_config config_armv8r_mpu_ut = {
    .data = &((struct mod_armv8r_mpu_config){
        .attributes_count = FWK_ARRAY_SIZE(mem_attributes),
        .attributes = mem_attributes,
        .region_count = FWK_ARRAY_SIZE(mem_regions),
        .regions = mem_regions,
    }),
};
