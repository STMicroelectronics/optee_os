/*
 * Arm SCP/MCP Software
 * Copyright (c) 2024, Arm Limited and Contributors. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include "fvp_baser_aemv8r_mmap.h"
#include "mod_gicx00.h"

#include <fwk_module.h>

const struct fwk_module_config config_gicx00 = {
    .data = &((struct mod_gicx00_config){
        .gicd_base = FVP_GICD_BASE,
        .gicr_base = FVP_GICR_BASE,
    }),
};
