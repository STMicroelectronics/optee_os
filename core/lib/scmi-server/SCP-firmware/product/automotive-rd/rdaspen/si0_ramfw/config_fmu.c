/*
 * Arm SCP/MCP Software
 * Copyright (c) 2025, Arm Limited and Contributors. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */
#include "si0_irq.h"
#include "si0_mmap.h"

#include <mod_fmu.h>

#include <fwk_log.h>
#include <fwk_module.h>

enum fmu_device {
    SI0_FMU_ROOT,
    SI0_FMU_1,
    SI0_FMU_2,
    SI0_FMU_3,
    SI0_FMU_4,
    SI0_GIC_FMU,
    SI0_RSE_CL0_MHU_FMU,
    SI0_CL0_RSE_MHU_FMU,
    SI0_PC0_CL0_MHU_FMU,
    SI0_CL0_PC0_MHU_FMU,
    SI0_PC1_CL0_MHU_FMU,
    SI0_CL0_PC1_MHU_FMU,
    SI0_PC2_CL0_MHU_FMU,
    SI0_CL0_PC2_MHU_FMU,
    SI0_PC3_CL0_MHU_FMU,
    SI0_CL0_PC3_MHU_FMU,
    SI0_FMU_COUNT,
};

static const struct fwk_element fmu_devices[SI0_FMU_COUNT + 1] = {
    [SI0_FMU_ROOT] = {
        .name = "fmu0",
        .data = &((struct mod_fmu_dev_config) {
            .base = SI0_FMU0_BASE,
            .parent = MOD_FMU_PARENT_NONE,
            .implementation = MOD_FMU_SYSTEM_IMPL,
        }),
    },
    [SI0_FMU_1] = {
        .name = "fmu1",
        .data = &((struct mod_fmu_dev_config) {
            .base = SI0_FMU1_BASE,
            .parent = SI0_FMU_ROOT,
            .parent_cr_index = 0,
            .parent_ncr_index = 1,
            .implementation = MOD_FMU_SYSTEM_IMPL,
        }),
    },
    [SI0_FMU_2] = {
        .name = "fmu2",
        .data = &((struct mod_fmu_dev_config) {
            .base = SI0_FMU2_BASE,
            .parent = SI0_FMU_ROOT,
            .parent_cr_index = 2,
            .parent_ncr_index = 3,
            .implementation = MOD_FMU_SYSTEM_IMPL,
        }),
    },
    [SI0_FMU_3] = {
        .name = "fmu3",
        .data = &((struct mod_fmu_dev_config) {
            .base = SI0_FMU3_BASE,
            .parent = SI0_FMU_ROOT,
            .parent_cr_index = 4,
            .parent_ncr_index = 5,
            .implementation = MOD_FMU_SYSTEM_IMPL,
        }),
    },
    [SI0_FMU_4] = {
        .name = "fmu4",
        .data = &((struct mod_fmu_dev_config) {
            .base = SI0_FMU4_BASE,
            .parent = SI0_FMU_ROOT,
            .parent_cr_index = 6,
            .parent_ncr_index = 7,
            .implementation = MOD_FMU_SYSTEM_IMPL,
        }),
    },
    [SI0_GIC_FMU] = {
        .name = "gic_fmu",
        .data = &((struct mod_fmu_dev_config) {
            .base = SI0_GIC_FMU_BASE,
            .parent = SI0_FMU_4,
            .parent_cr_index = 204,
            .parent_ncr_index = 203,
            .implementation = MOD_FMU_GIC_MHU_IMPL,
        }),
    },
    [SI0_RSE_CL0_MHU_FMU] = {
        .name = "rse_cl0_mhu_fmu",
        .data = &((struct mod_fmu_dev_config) {
            .base = SI0_RSE_CL0_MHU_FMU_BASE,
            .parent = SI0_FMU_4,
            .parent_cr_index = 0,
            .parent_ncr_index = 1,
            .implementation = MOD_FMU_GIC_MHU_IMPL,
        }),
    },
    [SI0_CL0_RSE_MHU_FMU] = {
        .name = "cl0_rse_mhu_fmu",
        .data = &((struct mod_fmu_dev_config) {
            .base = SI0_CL0_RSE_MHU_FMU_BASE,
            .parent = SI0_FMU_4,
            .parent_cr_index = 2,
            .parent_ncr_index = 3,
            .implementation = MOD_FMU_GIC_MHU_IMPL,
        }),
    },
    [SI0_PC0_CL0_MHU_FMU] = {
        .name = "pc0_cl0_mhu_fmu",
        .data = &((struct mod_fmu_dev_config) {
            .base = SI0_PC0_CL0_MHU_FMU_BASE,
            .parent = SI0_FMU_4,
            .parent_cr_index = 4,
            .parent_ncr_index = 5,
            .implementation = MOD_FMU_GIC_MHU_IMPL,
        }),
    },
    [SI0_CL0_PC0_MHU_FMU] = {
        .name = "cl0_pc0_mhu_fmu",
        .data = &((struct mod_fmu_dev_config) {
            .base = SI0_CL0_PC0_MHU_FMU_BASE,
            .parent = SI0_FMU_4,
            .parent_cr_index = 6,
            .parent_ncr_index = 7,
            .implementation = MOD_FMU_GIC_MHU_IMPL,
        }),
    },
    [SI0_PC1_CL0_MHU_FMU] = {
        .name = "pc1_cl0_mhu_fmu",
        .data = &((struct mod_fmu_dev_config) {
            .base = SI0_PC1_CL0_MHU_FMU_BASE,
            .parent = SI0_FMU_4,
            .parent_cr_index = 8,
            .parent_ncr_index = 9,
            .implementation = MOD_FMU_GIC_MHU_IMPL,
        }),
    },
    [SI0_CL0_PC1_MHU_FMU] = {
        .name = "cl0_pc1_mhu_fmu",
        .data = &((struct mod_fmu_dev_config) {
            .base = SI0_CL0_PC1_MHU_FMU_BASE,
            .parent = SI0_FMU_4,
            .parent_cr_index = 10,
            .parent_ncr_index = 11,
            .implementation = MOD_FMU_GIC_MHU_IMPL,
        }),
    },
    [SI0_PC2_CL0_MHU_FMU] = {
        .name = "pc2_cl0_mhu_fmu",
        .data = &((struct mod_fmu_dev_config) {
            .base = SI0_PC2_CL0_MHU_FMU_BASE,
            .parent = SI0_FMU_4,
            .parent_cr_index = 12,
            .parent_ncr_index = 13,
            .implementation = MOD_FMU_GIC_MHU_IMPL,
        }),
    },
    [SI0_CL0_PC2_MHU_FMU] = {
        .name = "cl0_pc2_mhu_fmu",
        .data = &((struct mod_fmu_dev_config) {
            .base = SI0_CL0_PC2_MHU_FMU_BASE,
            .parent = SI0_FMU_4,
            .parent_cr_index = 14,
            .parent_ncr_index = 15,
            .implementation = MOD_FMU_GIC_MHU_IMPL,
        }),
    },
    [SI0_PC3_CL0_MHU_FMU] = {
        .name = "pc3_cl0_mhu_fmu",
        .data = &((struct mod_fmu_dev_config) {
            .base = SI0_PC3_CL0_MHU_FMU_BASE,
            .parent = SI0_FMU_4,
            .parent_cr_index = 16,
            .parent_ncr_index = 17,
            .implementation = MOD_FMU_GIC_MHU_IMPL,
        }),
    },
    [SI0_CL0_PC3_MHU_FMU] = {
        .name = "cl0_pc3_mhu_fmu",
        .data = &((struct mod_fmu_dev_config) {
            .base = SI0_CL0_PC3_MHU_FMU_BASE,
            .parent = SI0_FMU_4,
            .parent_cr_index = 18,
            .parent_ncr_index = 19,
            .implementation = MOD_FMU_GIC_MHU_IMPL,
        }),
    },
    [SI0_FMU_COUNT] = {0},
};

/*
 * Configuration for the debugger CLI module
 */
struct fwk_module_config config_fmu = {
    .data =
        &(struct mod_fmu_config){
            .irq_critical = CL0_FMU_CRITICAL,
            .irq_non_critical = CL0_FMU_NON_CRITICAL,
            .timer_id = FWK_ID_ELEMENT_INIT(FWK_MODULE_IDX_TIMER, 0),
        },
    .elements = FWK_MODULE_STATIC_ELEMENTS_PTR(fmu_devices),
};
