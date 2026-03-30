/*
 * Arm SCP/MCP Software
 * Copyright (c) 2025, Arm Limited and Contributors. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 * Description:
 *     Configuration data for module 'system_pll'.
 */

#include "si0_clock.h"
#include "si0_exp_mmap.h"

#include <mod_system_pll.h>

#include <fwk_element.h>
#include <fwk_id.h>
#include <fwk_macros.h>
#include <fwk_module.h>

static const struct fwk_element sys_pll_table[] = {
    [CFGD_MOD_SYSTEM_PLL_EIDX_CORE] = {
        .name = "CORE",
        .data = &((struct mod_system_pll_dev_config) {
            .control_reg = (void *)SI0_PLL_CORE,
            .status_reg = (void *)SI0_PLL_CORE,
            .lock_flag_mask = SI0_PLL_LOCK_MASK,
            .period_shift = 1UL,
            .initial_rate = CLOCK_RATE_CLUSTERPLLCLK,
            .min_rate = MOD_SYSTEM_PLL_MIN_RATE,
            .max_rate = MOD_SYSTEM_PLL_MAX_RATE,
            .min_step = MOD_SYSTEM_PLL_MIN_INTERVAL,
        }),
    },
    [CFGD_MOD_SYSTEM_PLL_EIDX_CORE0] = {
        .name = "CORE0",
        .data = &((struct mod_system_pll_dev_config) {
            .control_reg = (void *)SI0_PLL_CORE0,
            .status_reg = (void *)SI0_PLL_CORE0,
            .lock_flag_mask = SI0_PLL_LOCK_MASK,
            .period_shift = 1UL,
            .initial_rate = CLOCK_RATE_COREPLL0CLK,
            .min_rate = MOD_SYSTEM_PLL_MIN_RATE,
            .max_rate = MOD_SYSTEM_PLL_MAX_RATE,
            .min_step = MOD_SYSTEM_PLL_MIN_INTERVAL,
        }),
    },
    [CFGD_MOD_SYSTEM_PLL_EIDX_CORE1] = {
        .name = "CORE1",
        .data = &((struct mod_system_pll_dev_config) {
            .control_reg = (void *)SI0_PLL_CORE1,
            .status_reg = (void *)SI0_PLL_CORE1,
            .lock_flag_mask = SI0_PLL_LOCK_MASK,
            .period_shift = 1UL,
            .initial_rate = CLOCK_RATE_COREPLL1CLK,
            .min_rate = MOD_SYSTEM_PLL_MIN_RATE,
            .max_rate = MOD_SYSTEM_PLL_MAX_RATE,
            .min_step = MOD_SYSTEM_PLL_MIN_INTERVAL,
        }),
    },
    [CFGD_MOD_SYSTEM_PLL_EIDX_SYS] = {
        .name = "SYS",
        .data = &((struct mod_system_pll_dev_config) {
            .control_reg = (void *)SI0_PLL_SYS,
            .status_reg = (void *)SI0_PLL_SYS,
            .lock_flag_mask = SI0_PLL_LOCK_MASK,
            .period_shift = 1UL,
            .initial_rate = CLOCK_RATE_SYSPLLCLK,
            .min_rate = MOD_SYSTEM_PLL_MIN_RATE,
            .max_rate = MOD_SYSTEM_PLL_MAX_RATE,
            .min_step = MOD_SYSTEM_PLL_MIN_INTERVAL,
        }),
    },
    [CFGD_MOD_SYSTEM_PLL_EIDX_PERIPH] = {
        .name = "PERIPH",
        .data = &((struct mod_system_pll_dev_config) {
            .control_reg = (void *)SI0_PLL_PERIPH,
            .status_reg = (void *)SI0_PLL_PERIPH,
            .lock_flag_mask = SI0_PLL_LOCK_MASK,
            .period_shift = 1UL,
            .initial_rate = CLOCK_RATE_PERIPHPLLCLK,
            .min_rate = MOD_SYSTEM_PLL_MIN_RATE,
            .max_rate = MOD_SYSTEM_PLL_MAX_RATE,
            .min_step = MOD_SYSTEM_PLL_MIN_INTERVAL,
        }),
    },
    [CFGD_MOD_SYSTEM_PLL_EIDX_GIC] = {
        .name = "GIC",
        .data = &((struct mod_system_pll_dev_config) {
            .control_reg = (void *)SI0_PLL_GIC,
            .status_reg = (void *)SI0_PLL_GIC,
            .lock_flag_mask = SI0_PLL_LOCK_MASK,
            .period_shift = 1UL,
            .initial_rate = CLOCK_RATE_GICPLLCLK,
            .min_rate = MOD_SYSTEM_PLL_MIN_RATE,
            .max_rate = MOD_SYSTEM_PLL_MAX_RATE,
            .min_step = MOD_SYSTEM_PLL_MIN_INTERVAL,
        }),
    },
    [CFGD_MOD_SYSTEM_PLL_EIDX_IOBLOCK] = {
        .name = "IOBLOCK",
        .data = &((struct mod_system_pll_dev_config) {
            .control_reg = (void *)SI0_PLL_IOBLOCK,
            .status_reg = (void *)SI0_PLL_IOBLOCK,
            .lock_flag_mask = SI0_PLL_LOCK_MASK,
            .period_shift = 1UL,
            .initial_rate = CLOCK_RATE_IOPLLCLK,
            .min_rate = MOD_SYSTEM_PLL_MIN_RATE,
            .max_rate = MOD_SYSTEM_PLL_MAX_RATE,
            .min_step = MOD_SYSTEM_PLL_MIN_INTERVAL,
        }),
    },
    [CFGD_MOD_SYSTEM_PLL_EIDX_RSE] = {
        .name = "RSE",
        .data = &((struct mod_system_pll_dev_config) {
            .control_reg = (void *)SI0_PLL_RSE,
            .status_reg = (void *)SI0_PLL_RSE,
            .lock_flag_mask = SI0_PLL_LOCK_MASK,
            .period_shift = 1UL,
            .initial_rate = CLOCK_RATE_RSEPLLCLK,
            .min_rate = MOD_SYSTEM_PLL_MIN_RATE,
            .max_rate = MOD_SYSTEM_PLL_MAX_RATE,
            .min_step = MOD_SYSTEM_PLL_MIN_INTERVAL,
        }),
    },
    [CFGD_MOD_SYSTEM_PLL_EIDX_SI] = {
        .name = "SI",
        .data = &((struct mod_system_pll_dev_config) {
            .control_reg = (void *)SI0_PLL_SI,
            .status_reg = (void *)SI0_PLL_SI,
            .lock_flag_mask = SI0_PLL_LOCK_MASK,
            .period_shift = 1UL,
            .initial_rate = CLOCK_RATE_SIPLLCLK,
            .min_rate = MOD_SYSTEM_PLL_MIN_RATE,
            .max_rate = MOD_SYSTEM_PLL_MAX_RATE,
            .min_step = MOD_SYSTEM_PLL_MIN_INTERVAL,
        }),
    },
    [CFGD_MOD_SYSTEM_PLL_EIDX_SMD] = {
        .name = "SMD",
        .data = &((struct mod_system_pll_dev_config) {
            .control_reg = (void *)SI0_PLL_SMD,
            .status_reg = (void *)SI0_PLL_SMD,
            .lock_flag_mask = SI0_PLL_LOCK_MASK,
            .period_shift = 1UL,
            .initial_rate = CLOCK_RATE_SMDPLLCLK,
            .min_rate = MOD_SYSTEM_PLL_MIN_RATE,
            .max_rate = MOD_SYSTEM_PLL_MAX_RATE,
            .min_step = MOD_SYSTEM_PLL_MIN_INTERVAL,
        }),
    },
    [CFGD_MOD_SYSTEM_PLL_EIDX_DBG] = {
        .name = "DBG",
        .data = &((struct mod_system_pll_dev_config) {
            .control_reg = (void *)SI0_PLL_DBG,
            .status_reg = (void *)SI0_PLL_DBG,
            .lock_flag_mask = SI0_PLL_LOCK_MASK,
            .period_shift = 1UL,
            .initial_rate = CLOCK_RATE_DBGPLLCLK,
            .min_rate = MOD_SYSTEM_PLL_MIN_RATE,
            .max_rate = MOD_SYSTEM_PLL_MAX_RATE,
            .min_step = MOD_SYSTEM_PLL_MIN_INTERVAL,
        }),
    },
    [CFGD_MOD_SYSTEM_PLL_EIDX_TRACE] = {
        .name = "TRACE",
        .data = &((struct mod_system_pll_dev_config) {
            .control_reg = (void *)SI0_PLL_TRACE,
            .status_reg = (void *)SI0_PLL_TRACE,
            .lock_flag_mask = SI0_PLL_LOCK_MASK,
            .period_shift = 1UL,
            .initial_rate = CLOCK_RATE_TRACEPLLCLK,
            .min_rate = MOD_SYSTEM_PLL_MIN_RATE,
            .max_rate = MOD_SYSTEM_PLL_MAX_RATE,
            .min_step = MOD_SYSTEM_PLL_MIN_INTERVAL,
        }),
    },
    [CFGD_MOD_SYSTEM_PLL_EIDX_COUNT] = { 0 }, /* Termination description. */
};

const struct fwk_module_config config_system_pll = {
    .elements = FWK_MODULE_STATIC_ELEMENTS_PTR(sys_pll_table),
};
