/*
 * Arm SCP/MCP Software
 * Copyright (c) 2025, Arm Limited and Contributors. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */
#include <mod_integration_test.h>

#include <fwk_id.h>
#include <fwk_module.h>

enum test_cases {
    TEST_TIMER,
    TEST_COUNT,
};

static const struct fwk_element config_integration_test_elements[] = {
    [TEST_TIMER] = {
        .name = "timer",
        .data = &(struct mod_integration_test_config){
            .test_id = FWK_ID_MODULE_INIT(FWK_MODULE_IDX_TEST_TIMER),
            .run_at_start = false,
            .num_test_cases = 1,
        },
    },
    [TEST_COUNT] = { 0 }
};

struct fwk_module_config config_integration_test = {
    .elements =
        FWK_MODULE_STATIC_ELEMENTS_PTR(config_integration_test_elements),
};
