/*
 * Arm SCP/MCP Software
 * Copyright (c) 2024, Arm Limited and Contributors. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include "scp_mmap.h"
#include "tc3_core.h"
#include "tc3_dvfs.h"
#include "tc3_timer.h"
#include "tc_amu.h"

#include <mod_amu_mmap.h>
#include <mod_mpmm_v2.h>

#include <fwk_macros.h>
#include <fwk_module.h>
#include <fwk_module_idx.h>

enum mpmm_v2_domain_idx {
    MPMM_V2_LITTLE_DOM_IDX,
    MPMM_V2_MID_DOM_IDX,
    MPMM_V2_BIG_DOM_IDX,
    MPMM_V2_DOM_COUNT,
};

enum cpu_idx {
    CORE0_IDX,
    CORE1_IDX,
    CORE2_IDX,
    CORE3_IDX,
    CORE4_IDX,
    CORE5_IDX,
    CORE6_IDX,
    CORE7_IDX
};

static const struct mod_mpmm_v2_core_config little_core_config[] = {
    [CORE0_IDX] = {
        .pd_id = FWK_ID_ELEMENT_INIT(FWK_MODULE_IDX_POWER_DOMAIN, CORE0_IDX),
        .mpmm_reg_base = SCP_MPMM_CORE_BASE(CORE0_IDX),
        .core_starts_online = true,
        .base_aux_counter_id = FWK_ID_SUB_ELEMENT_INIT(
            FWK_MODULE_IDX_AMU_MMAP, CORE0_IDX, AMEVCNTR1_AUX0),
    },
    [CORE1_IDX] = {
        .pd_id = FWK_ID_ELEMENT_INIT(FWK_MODULE_IDX_POWER_DOMAIN, CORE1_IDX),
        .mpmm_reg_base = SCP_MPMM_CORE_BASE(CORE1_IDX),
        .core_starts_online = false,
        .base_aux_counter_id = FWK_ID_SUB_ELEMENT_INIT(
            FWK_MODULE_IDX_AMU_MMAP, CORE1_IDX, AMEVCNTR1_AUX0),
    },
    [CORE2_IDX] = {
        .pd_id = FWK_ID_ELEMENT_INIT(FWK_MODULE_IDX_POWER_DOMAIN, CORE2_IDX),
        .mpmm_reg_base = SCP_MPMM_CORE_BASE(CORE2_IDX),
        .core_starts_online = false,
        .base_aux_counter_id = FWK_ID_SUB_ELEMENT_INIT(
            FWK_MODULE_IDX_AMU_MMAP, CORE2_IDX, AMEVCNTR1_AUX0),
    },
    [CORE3_IDX] = {
        .pd_id = FWK_ID_ELEMENT_INIT(FWK_MODULE_IDX_POWER_DOMAIN, CORE3_IDX),
        .mpmm_reg_base = SCP_MPMM_CORE_BASE(CORE3_IDX),
        .core_starts_online = false,
        .base_aux_counter_id = FWK_ID_SUB_ELEMENT_INIT(
            FWK_MODULE_IDX_AMU_MMAP, CORE3_IDX, AMEVCNTR1_AUX0),
    },
};

static const struct mod_mpmm_v2_core_config mid_core_config[] = {
    [CORE0_IDX] = {
        .pd_id = FWK_ID_ELEMENT_INIT(FWK_MODULE_IDX_POWER_DOMAIN, CORE4_IDX),
        .mpmm_reg_base = SCP_MPMM_CORE_BASE(CORE4_IDX),
        .core_starts_online = false,
        .base_aux_counter_id = FWK_ID_SUB_ELEMENT_INIT(
            FWK_MODULE_IDX_AMU_MMAP, CORE4_IDX, AMEVCNTR1_AUX0),
    },
    [CORE1_IDX] = {
        .pd_id = FWK_ID_ELEMENT_INIT(FWK_MODULE_IDX_POWER_DOMAIN, CORE5_IDX),
        .mpmm_reg_base = SCP_MPMM_CORE_BASE(CORE5_IDX),
        .core_starts_online = false,
        .base_aux_counter_id = FWK_ID_SUB_ELEMENT_INIT(
            FWK_MODULE_IDX_AMU_MMAP, CORE5_IDX, AMEVCNTR1_AUX0),
    },
    [CORE2_IDX] = {
        .pd_id = FWK_ID_ELEMENT_INIT(FWK_MODULE_IDX_POWER_DOMAIN, CORE6_IDX),
        .mpmm_reg_base = SCP_MPMM_CORE_BASE(CORE6_IDX),
        .core_starts_online = false,
        .base_aux_counter_id = FWK_ID_SUB_ELEMENT_INIT(
            FWK_MODULE_IDX_AMU_MMAP, CORE6_IDX, AMEVCNTR1_AUX0),
    },
};

static const struct mod_mpmm_v2_core_config big_core_config[1] = {
    [CORE0_IDX] = {
        .pd_id = FWK_ID_ELEMENT_INIT(FWK_MODULE_IDX_POWER_DOMAIN, CORE7_IDX),
        .mpmm_reg_base = SCP_MPMM_CORE_BASE(CORE7_IDX),
        .core_starts_online = false,
        .base_aux_counter_id = FWK_ID_SUB_ELEMENT_INIT(
            FWK_MODULE_IDX_AMU_MMAP, CORE7_IDX, AMEVCNTR1_AUX0),
    },
};

static const struct mod_mpmm_v2_domain_config little_domain_conf[2] = {
    [0] = {
#ifdef BUILD_HAS_MOD_PERF_CONTROLLER
        .perf_id = FWK_ID_ELEMENT_INIT(
            FWK_MODULE_IDX_PERF_CONTROLLER, DVFS_ELEMENT_IDX_GROUP_LITTLE),
#else
        .perf_id = FWK_ID_ELEMENT_INIT(
            FWK_MODULE_IDX_DVFS, DVFS_ELEMENT_IDX_GROUP_LITTLE),
#endif
        .max_power = 447,
        .min_power = 112,
        .gear_weights = (uint32_t[3]) {
            [0] = 100,
            [1] = 93,
            [2] = 86,
        },
        .base_throtl_count = 10,
        .num_of_gears = 3,
        .core_config = little_core_config,
    },
    [1] = {0},
};

static const struct mod_mpmm_v2_domain_config mid_domain_conf[2] = {
    [0] = {
#ifdef BUILD_HAS_MOD_PERF_CONTROLLER
        .perf_id = FWK_ID_ELEMENT_INIT(
            FWK_MODULE_IDX_PERF_CONTROLLER, DVFS_ELEMENT_IDX_GROUP_MID),
#else
        .perf_id = FWK_ID_ELEMENT_INIT(
            FWK_MODULE_IDX_DVFS, DVFS_ELEMENT_IDX_GROUP_MID),
#endif
        .max_power = 1183,
        .min_power = 527,
        .gear_weights = (uint32_t[3]) {
            [0] = 100,
            [1] = 90,
            [2] = 84,
        },
        .base_throtl_count = 10,
        .num_of_gears = 3,
        .core_config = mid_core_config,
    },
    [1] = {0},
};

static const struct mod_mpmm_v2_domain_config big_domain_conf[2] = {
    [0] = {
#ifdef BUILD_HAS_MOD_PERF_CONTROLLER
        .perf_id = FWK_ID_ELEMENT_INIT(
            FWK_MODULE_IDX_PERF_CONTROLLER, DVFS_ELEMENT_IDX_GROUP_BIG),
#else
        .perf_id = FWK_ID_ELEMENT_INIT(
            FWK_MODULE_IDX_DVFS, DVFS_ELEMENT_IDX_GROUP_BIG),
#endif
        .max_power = 2898,
        .min_power = 1989,
        .gear_weights = (uint32_t[3]) {
            [0] = 100,
            [1] = 100,
            [2] = 70,
        },
        .base_throtl_count = 10,
        .num_of_gears = 3,
        .core_config = big_core_config,
    },
    [1] = {0},
};

static const struct fwk_element mpmm_v2_element_table[4] = {
    [MPMM_V2_LITTLE_DOM_IDX] = {
        .name = "MPMM_LITTLE_ELEM",
        .sub_element_count = 4,
        .data = little_domain_conf,
    },
    [MPMM_V2_MID_DOM_IDX] = {
        .name = "MPMM_MID_ELEM",
        .sub_element_count = 3,
        .data = mid_domain_conf,
    },
    [MPMM_V2_BIG_DOM_IDX] = {
        .name = "MPMM_BIG_ELEM",
        .sub_element_count = 1,
        .data = big_domain_conf,
    },
    [MPMM_V2_DOM_COUNT] = { 0 },
};

static const struct fwk_element *mpmm_get_element_table(fwk_id_t module_id)
{
    return mpmm_v2_element_table;
}
const struct fwk_module_config config_mpmm_v2 = {
    .elements = FWK_MODULE_DYNAMIC_ELEMENTS(mpmm_get_element_table),
    .data = (const void *)(&(fwk_id_t)FWK_ID_API_INIT(
        FWK_MODULE_IDX_AMU_MMAP,
        MOD_AMU_MMAP_API_IDX_AMU)),
};
