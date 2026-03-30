/*
 * Arm SCP/MCP Software
 * Copyright (c) 2025, Arm Limited and Contributors. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <mod_fmu.h>

#include <fwk_id.h>
#include <fwk_module.h>
#include <fwk_module_idx.h>

enum fmu_device {
    SCP_FMU_ROOT,
    SCP_FMU_1,
    SCP_FMU_2,
    SCP_FMU_COUNT,
};

static uint8_t fmu_reg[SCP_FMU_COUNT][64 * FWK_KIB] = { 0 };

static const struct fwk_element fmu_devices[SCP_FMU_COUNT + 1] = {
    [SCP_FMU_ROOT] = {
        .name = "fmu0",
        .data = &((struct mod_fmu_dev_config) {
            .base = (uintptr_t)fmu_reg[SCP_FMU_ROOT],
            .parent = MOD_FMU_PARENT_NONE,
        }),
    },
    [SCP_FMU_1] = {
        .name = "fmu1",
        .data = &((struct mod_fmu_dev_config) {
            .base = (uintptr_t)fmu_reg[SCP_FMU_1],
            .parent = SCP_FMU_ROOT,
            .parent_cr_index = 0,
            .parent_ncr_index = 1,
        }),
    },
    [SCP_FMU_2] = {
        .name = "fmu2",
        .data = &((struct mod_fmu_dev_config) {
            .base = (uintptr_t)fmu_reg[SCP_FMU_2],
            .parent = SCP_FMU_ROOT,
            .parent_cr_index = 2,
            .parent_ncr_index = 3,
        }),
    },
    [SCP_FMU_COUNT] = {0},
};

struct fwk_module_config config_fmu = {
    .data =
        &(struct mod_fmu_config){
            .irq_critical = 12,
            .irq_non_critical = 13,
        },
    .elements = FWK_MODULE_STATIC_ELEMENTS_PTR(fmu_devices),
};
