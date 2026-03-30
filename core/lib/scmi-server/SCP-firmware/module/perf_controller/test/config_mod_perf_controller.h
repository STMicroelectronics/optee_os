/*
 * Arm SCP/MCP Software
 * Copyright (c) 2024-2025, Arm Limited and Contributors. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <Mockfwk_module.h>

#include <mod_perf_controller.h>

#include <fwk_element.h>
#include <fwk_module_idx.h>

#define MAX_LIMITER_PER_DOMAIN 4U

enum test_perf_controller_domain_idx {
    TEST_BIG_DOMAIN,
    TEST_LITTLE_DOMAIN,
    TEST_DOMAIN_COUNT,
};

struct fwk_element domain_config[TEST_DOMAIN_COUNT] = {
    [TEST_BIG_DOMAIN] = {
        .sub_element_count = MAX_LIMITER_PER_DOMAIN,
        .data = &(struct mod_perf_controller_domain_config) {
            .performance_driver_id = FWK_ID_ELEMENT_INIT(FWK_MODULE_IDX_TEST_PERF_CONT_DRVR,TEST_BIG_DOMAIN),
            .performance_driver_api_id = FWK_ID_API_INIT(FWK_MODULE_IDX_TEST_PERF_CONT_DRVR,TEST_BIG_DOMAIN),
            .power_model_id = FWK_ID_ELEMENT_INIT(FWK_MODULE_IDX_TEST_PERF_MODEL_DRVR,TEST_BIG_DOMAIN),
            .power_model_api_id = FWK_ID_API_INIT(FWK_MODULE_IDX_TEST_PERF_MODEL_DRVR,TEST_BIG_DOMAIN),
        },
    },
    [TEST_LITTLE_DOMAIN] = {
        .sub_element_count = 1U,
        .data = &(struct mod_perf_controller_domain_config) {
            .performance_driver_id = FWK_ID_ELEMENT_INIT(FWK_MODULE_IDX_TEST_PERF_CONT_DRVR,TEST_LITTLE_DOMAIN),
            .performance_driver_api_id = FWK_ID_API_INIT(FWK_MODULE_IDX_TEST_PERF_CONT_DRVR,TEST_LITTLE_DOMAIN),
            .power_model_id = FWK_ID_ELEMENT_INIT(FWK_MODULE_IDX_TEST_PERF_MODEL_DRVR,TEST_LITTLE_DOMAIN),
            .power_model_api_id = FWK_ID_API_INIT(FWK_MODULE_IDX_TEST_PERF_MODEL_DRVR,TEST_LITTLE_DOMAIN),
        },
    },
};
