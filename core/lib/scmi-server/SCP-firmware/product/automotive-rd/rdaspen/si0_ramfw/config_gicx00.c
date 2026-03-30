/*
 * Arm SCP/MCP Software
 * Copyright (c) 2025, Arm Limited and Contributors. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include "mod_gicx00.h"
#include "si0_mmap.h"

#include <fwk_module.h>

const struct fwk_module_config config_gicx00 = {
    .data = &((struct mod_gicx00_config){
        .gicd_base = SI0_GICD_BASE,
        .gicr_base = SI0_GICR_BASE,
    }),
};
