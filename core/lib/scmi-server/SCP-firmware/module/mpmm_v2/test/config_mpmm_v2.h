/*
 * Arm SCP/MCP Software
 * Copyright (c) 2024, Arm Limited and Contributors. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */
#include <internal/mpmm_v2.h>

#include <fwk_macros.h>
#include <fwk_module_idx.h>

#define NUM_OF_MPMM_GEARS       1
#define DEFAULT_MAX_POWER_LIMIT 1200
#define DEFAULT_MIN_POWER_LIMIT 300

enum amu_counter {
    AMU_CORE,
    AMU_CONST,
    AMU_INST_RET,
    AMU_MEM_STALL,
    AMU_AUX0,
    AMU_AUX1,
    AMU_AUX2,
    AMU_COUNT,
};

enum cpu_idx {
    CORE0_IDX,
    CORE1_IDX,
    CORE2_IDX,
    CORE_IDX_COUNT,
};

enum fake_mpmm_v2_domain_idx {
    MPMM_V2_DOM_DEFAULT,
    MPMM_V2_DOM_MAX_GEAR_COUNT,
    MPMM_V2_DOM_NUM_GEAR_NOT_EQUAL,
    MPMM_V2_DOM_TWO_GEAR_COUNTER,
    MPMM_V2_DOM_MPMM_ENABLED,
    MPMM_V2_DOM_COUNT,
};

static struct mpmm_reg fake_mpmm_reg[CORE_IDX_COUNT] = {
    [CORE0_IDX] = {
        .PPMCR = (NUM_OF_MPMM_GEARS << MPMM_PPMCR_NUM_GEARS_POS),
        .MPMMCR = 0,
    },
    [CORE1_IDX] = {
        .PPMCR = (NUM_OF_MPMM_GEARS << MPMM_PPMCR_NUM_GEARS_POS),
        .MPMMCR = 0,
    },
    [CORE2_IDX] = {
        .PPMCR = (NUM_OF_MPMM_GEARS << MPMM_PPMCR_NUM_GEARS_POS),
        .MPMMCR = 1,
    },

};

static const struct mod_mpmm_v2_core_config fake_core_config[2] = {
    [CORE0_IDX] = {
        .pd_id = FWK_ID_ELEMENT_INIT(FWK_MODULE_IDX_POWER_DOMAIN, 0),
        .mpmm_reg_base = (uintptr_t)&fake_mpmm_reg[CORE0_IDX],
        .core_starts_online = true,
        .base_aux_counter_id = FWK_ID_SUB_ELEMENT_INIT(
            FWK_MODULE_IDX_AMU_MMAP, CORE0_IDX, AMU_AUX0),
    },
    [CORE1_IDX] = {
        .pd_id = FWK_ID_ELEMENT_INIT(FWK_MODULE_IDX_POWER_DOMAIN, 0),
        .mpmm_reg_base = (uintptr_t)&fake_mpmm_reg[CORE1_IDX],
        .core_starts_online = true,
        .base_aux_counter_id = FWK_ID_SUB_ELEMENT_INIT(
            FWK_MODULE_IDX_AMU_MMAP, CORE1_IDX, AMU_AUX0),
    },
};

static const struct mod_mpmm_v2_core_config fake_core_config_mpmm_enabled[1] = {
    [CORE0_IDX] = {
        .pd_id = FWK_ID_ELEMENT_INIT(FWK_MODULE_IDX_POWER_DOMAIN, 0),
        .mpmm_reg_base = (uintptr_t)&fake_mpmm_reg[CORE2_IDX],
        .core_starts_online = true,
        .base_aux_counter_id = FWK_ID_SUB_ELEMENT_INIT(
            FWK_MODULE_IDX_AMU_MMAP, CORE0_IDX, AMU_AUX0),
    },
};

static struct mod_mpmm_v2_domain_config fake_dom_conf[MPMM_V2_DOM_COUNT] = {
    [MPMM_V2_DOM_DEFAULT] = {
        .core_config = fake_core_config,
        .max_power = DEFAULT_MAX_POWER_LIMIT,
        .min_power = DEFAULT_MIN_POWER_LIMIT,
        .gear_weights = (uint32_t []) {
            [0] = 100,
        },
        .num_of_gears = 1,
        .perf_id = FWK_ID_ELEMENT_INIT(FWK_MODULE_IDX_PERF_CONTROLLER, 0),
    },
    [MPMM_V2_DOM_MAX_GEAR_COUNT] = {
        .core_config = fake_core_config,
        .max_power = DEFAULT_MAX_POWER_LIMIT,
        .min_power = DEFAULT_MIN_POWER_LIMIT,
        .gear_weights = (uint32_t []) {
            [0] = 100,
            [1] = 50,
        },
        .base_throtl_count = 0,
        .num_of_gears = (MPMM_MAX_GEAR_COUNT + 1),
        .perf_id = FWK_ID_ELEMENT_INIT(FWK_MODULE_IDX_PERF_CONTROLLER, 0),
    },
    [MPMM_V2_DOM_NUM_GEAR_NOT_EQUAL] = {
        .core_config = fake_core_config,
        .max_power = DEFAULT_MAX_POWER_LIMIT,
        .min_power = DEFAULT_MIN_POWER_LIMIT,
        .gear_weights = (uint32_t []) {
            [0] = 100,
            [1] = 50,
        },
        .num_of_gears = (NUM_OF_MPMM_GEARS + 1),
        .perf_id = FWK_ID_ELEMENT_INIT(FWK_MODULE_IDX_PERF_CONTROLLER, 0),
    },
    [MPMM_V2_DOM_TWO_GEAR_COUNTER] = {
        .core_config = fake_core_config,
        .max_power = DEFAULT_MAX_POWER_LIMIT,
        .min_power = DEFAULT_MIN_POWER_LIMIT,
        .gear_weights = (uint32_t []) {
            [0] = 100,
            [1] = 25,
        },
        .num_of_gears = 2,
        .perf_id = FWK_ID_ELEMENT_INIT(FWK_MODULE_IDX_PERF_CONTROLLER, 0),
    },
    [MPMM_V2_DOM_MPMM_ENABLED] = {
        .core_config = fake_core_config_mpmm_enabled,
        .max_power = DEFAULT_MAX_POWER_LIMIT,
        .min_power = DEFAULT_MIN_POWER_LIMIT,
        .gear_weights = (uint32_t []) {
            [0] = 100,
        },
        .base_throtl_count = 10,
        .num_of_gears = 1,
        .perf_id = FWK_ID_ELEMENT_INIT(FWK_MODULE_IDX_PERF_CONTROLLER, 0),
    },
};
