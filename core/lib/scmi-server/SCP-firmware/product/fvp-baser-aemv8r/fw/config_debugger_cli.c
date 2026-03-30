/*
 * Arm SCP/MCP Software
 * Copyright (c) 2025, Arm Limited and Contributors. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <fvp_baser_cfgd_timer.h>

#include <mod_debugger_cli.h>

#include <fwk_module_idx.h>

static const struct mod_debugger_cli_module_config debugger_cli_data = {
    .alarm_id = FWK_ID_SUB_ELEMENT_INIT(
        FWK_MODULE_IDX_TIMER,
        0,
        FVP_BASE_R_TIMER_ALARM_IDX_DEBUGGER),
    .poll_period = 100000
};

struct fwk_module_config config_debugger_cli = {
    .data = &debugger_cli_data,
};
