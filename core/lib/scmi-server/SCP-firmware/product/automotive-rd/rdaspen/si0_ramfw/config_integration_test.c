/*
 * Arm SCP/MCP Software
 * Copyright (c) 2025, Arm Limited and Contributors. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */
#include <mod_integration_test.h>

#include <fwk_module.h>

enum integration_test {
    TEST_FMU,
    TEST_SSU,
    TEST_SBISTC,
    TEST_COUNT,
};

static const struct fwk_element config_integration_test_elements[] = {
    [TEST_FMU] = {
        .name = "fmu",
        .data = &(struct mod_integration_test_config){
            .test_id = FWK_ID_MODULE_INIT(FWK_MODULE_IDX_TEST_FMU),
            .run_at_start = false,
            .num_test_cases = 14,
        },
    },
    [TEST_SSU] = {
        .name = "ssu",
        .data = &(struct mod_integration_test_config){
            .test_id = FWK_ID_MODULE_INIT(FWK_MODULE_IDX_TEST_SSU),
            .run_at_start = false,
            .num_test_cases = 1,
        },
    },
    [TEST_SBISTC] = {
        .name = "sbistc",
        .data = &(struct mod_integration_test_config){
            .test_id = FWK_ID_MODULE_INIT(FWK_MODULE_IDX_TEST_SBISTC),
            .run_at_start = false,
            .num_test_cases = 32,
        },
    },
    [TEST_COUNT] = {0},
};

/*
 * Configuration for the integration test module
 */
struct fwk_module_config config_integration_test = {
    .elements =
        FWK_MODULE_STATIC_ELEMENTS_PTR(config_integration_test_elements),
};
