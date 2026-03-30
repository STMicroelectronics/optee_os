/*
 * Arm SCP/MCP Software
 * Copyright (c) 2023-2025, Arm Limited and Contributors. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 * Description:
 *      System Power unit test configuration.
 */

#include <mod_ppu_v1.h>

#include <fwk_id.h>
#include <fwk_interrupt.h>
#include <fwk_macros.h>
#include <fwk_module.h>
#include <fwk_module_idx.h>

#define PD_COUNT PD_PPU_IDX_COUNT

enum pd_ppu_idx {
    PD_PPU_IDX_0,
    PD_PPU_IDX_1,
    PD_PPU_IDX_2,
    PD_PPU_IDX_3,
    PD_PPU_IDX_COUNT,
    PD_PPU_IDX_NONE = UINT32_MAX
};

/* For cores, point to their cluster element. */
#define CLUSTER0_EID FWK_ID_ELEMENT_INIT(FWK_MODULE_IDX_PPU_V1, PD_PPU_IDX_2)

/* Optional dummy base addresses for host UT (never dereferenced by mocks). */
#define REG_BASE_CLUSTER0 ((uintptr_t)0xDEAD0000u)
#define REG_BASE_SYSTOP   ((uintptr_t)0xFEED0000u)

static const struct mod_ppu_v1_pd_config pd_ppu_ctx_config[PD_PPU_IDX_COUNT] = {
    [PD_PPU_IDX_0] = {
        .pd_type = MOD_PD_TYPE_CORE,
        .alarm_id = FWK_ID_SUB_ELEMENT_INIT(FWK_MODULE_IDX_TIMER, PD_PPU_IDX_0, 0),
        .alarm_delay = 10,
    },
    [PD_PPU_IDX_1] = {
        .pd_type = MOD_PD_TYPE_CORE,
        .alarm_id = FWK_ID_SUB_ELEMENT_INIT(FWK_MODULE_IDX_TIMER, PD_PPU_IDX_1, 0),
        .alarm_delay = 10,
    },
    [PD_PPU_IDX_2] = {
        .pd_type   = MOD_PD_TYPE_CLUSTER,
        .enable_opmode_support = true,
        .enable_opmode_dynamic_policy = true,
        .use_opmode_irqs = true,
        .min_op_mode = PPU_V1_OPMODE_00,
        .ppu = {
            .reg_base = REG_BASE_CLUSTER0,
            .irq = FWK_INTERRUPT_NONE,
        },
        .opmode_time_out = 100000,
    },
    [PD_PPU_IDX_3] = {
        .pd_type   = MOD_PD_TYPE_SYSTEM,
        .enable_opmode_support = true,
        .enable_opmode_dynamic_policy = true,
        .use_opmode_irqs = true,
        .min_op_mode = PPU_V1_OPMODE_00,
        .ppu = {
            .reg_base = REG_BASE_SYSTOP,
            .irq = FWK_INTERRUPT_NONE,
        },
        .default_power_on = false,
        .opmode_time_out = 100000,
    },
};

static struct mod_ppu_v1_config ppu_v1_config_data_ut = {
    .num_of_cores_in_cluster = 2,
    .is_cluster_ppu_dynamic_mode_configured = true,
};
