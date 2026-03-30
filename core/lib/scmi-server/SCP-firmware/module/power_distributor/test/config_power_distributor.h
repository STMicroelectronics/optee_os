/*
 * Arm SCP/MCP Software
 * Copyright (c) 2024-2025, Arm Limited and Contributors. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <mod_power_distributor.h>

#include <fwk_element.h>
#include <fwk_id.h>
#include <fwk_module_idx.h>

enum test_power_distributor_domain_idx {
    TEST_DOMAIN_SOC,
    TEST_DOMAIN_CPU,
    TEST_DOMAIN_CPU_BIG,
    TEST_DOMAIN_CPU_LITTLE,
    TEST_DOMAIN_GPU,
    TEST_DOMAIN_COUNT,
    TEST_DOMAIN_NONE = MOD_POWER_DISTRIBUTOR_DOMAIN_PARENT_IDX_NONE,
};

enum test_power_controller_api_idx {
    TEST_CONTROLLER_API_SOC,
    TEST_CONTROLLER_API_CPU,
    TEST_CONTROLLER_API_CPU_BIG,
    TEST_CONTROLLER_API_CPU_LITTLE,
    TEST_CONTROLLER_API_GPU,
    TEST_CONTROLLER_API_COUNT,
};

struct mod_power_distributor_domain_config
    test_power_distributor_domain_config[TEST_DOMAIN_COUNT] = {
    [TEST_DOMAIN_SOC] = {
        .parent_idx = TEST_DOMAIN_NONE,
        .controller_api_id = FWK_ID_API_INIT(
            FWK_MODULE_IDX_CONTROLLER,
            TEST_CONTROLLER_API_SOC),
    },
    [TEST_DOMAIN_CPU] = {
        .parent_idx = TEST_DOMAIN_SOC,
    },
    [TEST_DOMAIN_CPU_BIG] = {
        .parent_idx = TEST_DOMAIN_CPU,
        .controller_id = FWK_ID_ELEMENT_INIT(FWK_MODULE_IDX_CONTROLLER,
                            TEST_DOMAIN_CPU_BIG),
        .controller_api_id = FWK_ID_API_INIT(
            FWK_MODULE_IDX_CONTROLLER,
            TEST_CONTROLLER_API_CPU_BIG),
    },
    [TEST_DOMAIN_CPU_LITTLE] = {
        .parent_idx = TEST_DOMAIN_CPU,
        .controller_id = FWK_ID_ELEMENT_INIT(FWK_MODULE_IDX_CONTROLLER,
                            TEST_DOMAIN_CPU_LITTLE),
        .controller_api_id = FWK_ID_API_INIT(
            FWK_MODULE_IDX_CONTROLLER,
            TEST_CONTROLLER_API_CPU_LITTLE),
    },
    [TEST_DOMAIN_GPU] = {
        .parent_idx = TEST_DOMAIN_SOC,
        .controller_id = FWK_ID_ELEMENT_INIT(FWK_MODULE_IDX_CONTROLLER,
                            TEST_DOMAIN_GPU),
        .controller_api_id = FWK_ID_API_INIT(
            FWK_MODULE_IDX_CONTROLLER,
            TEST_CONTROLLER_API_GPU),
    },
};
