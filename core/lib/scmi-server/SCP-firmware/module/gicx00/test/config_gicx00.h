/*
 * Arm SCP/MCP Software
 * Copyright (c) 2024, Arm Limited and Contributors. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <mod_gicx00.h>

#include <fwk_macros.h>
#include <fwk_module.h>

#define GICD_SIZE (256 * FWK_KIB)
#define GICR_SIZE (256 * FWK_KIB)

static uint8_t gicd_reg[GICD_SIZE];
static uint8_t gicr_reg[GICR_SIZE];

const struct fwk_module_config config_gicx00_ut = {
    .data = &((struct mod_gicx00_config){
        .gicd_base = (uintptr_t)&gicd_reg,
        .gicr_base = (uintptr_t)&gicr_reg,
    }),
};
