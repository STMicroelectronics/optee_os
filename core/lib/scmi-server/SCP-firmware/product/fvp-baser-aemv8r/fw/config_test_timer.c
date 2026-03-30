/*
 * Arm SCP/MCP Software
 * Copyright (c) 2025, Arm Limited and Contributors. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <fvp_baser_cfgd_timer.h>

#include <mod_test_timer.h>

#include <fwk_id.h>
#include <fwk_module.h>
#include <fwk_module_idx.h>

static const struct mod_test_timer_config test_timer_config = {
    .alarm_id = FWK_ID_SUB_ELEMENT_INIT(
        FWK_MODULE_IDX_TIMER,
        0,
        FVP_BASE_R_TIMER_ALARM_IDX_TEST_TIMER),
};

struct fwk_module_config config_test_timer = {
    .data = &test_timer_config,
};
