/*
 * Arm SCP/MCP Software
 * Copyright (c) 2024-2025, Arm Limited and Contributors. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 * Unit test for frameworkfunction in mod_perf_controller.c
 */

#include "scp_unity.h"
#include "unity.h"

#include <Mockfwk_mm.h>
#include <config_mod_perf_controller.h>
#include <perf_controller.h>

#include <fwk_module_idx.h>

#include UNIT_TEST_SRC

struct mod_perf_controller_domain_ctx test_domain_ctx[TEST_DOMAIN_COUNT];

void setUp(void)
{
    perf_controller_ctx.domain_ctx_table = test_domain_ctx;
}

void tearDown(void)
{
    Mockfwk_mm_Verify();
    Mockfwk_mm_Destroy();
}

void test_init_invalid_element_count(void)
{
    int status;
    unsigned int element_count = 0U;
    fwk_id_t module_id = FWK_ID_MODULE_INIT(FWK_MODULE_IDX_PERF_CONTROLLER);

    status = mod_perf_controller_init(module_id, element_count, NULL);

    TEST_ASSERT_EQUAL(status, FWK_E_PARAM);
}

void test_init_success(void)
{
    int status;
    unsigned int element_count = TEST_DOMAIN_COUNT;
    fwk_id_t module_id = FWK_ID_MODULE_INIT(FWK_MODULE_IDX_PERF_CONTROLLER);
    void *allocated_memory_ptr = (void *)1234;

    fwk_mm_calloc_ExpectAndReturn(
        element_count,
        sizeof(struct mod_perf_controller_domain_ctx),
        allocated_memory_ptr);

    status = mod_perf_controller_init(module_id, element_count, NULL);

    TEST_ASSERT_EQUAL_PTR(
        perf_controller_ctx.domain_ctx_table, allocated_memory_ptr);
    TEST_ASSERT_EQUAL(status, FWK_SUCCESS);
}

void test_element_init_invalid_sub_element_count(void)
{
    int status;
    unsigned int sub_element_count = 0U;
    fwk_id_t element_id = FWK_ID_ELEMENT_INIT(
        FWK_MODULE_IDX_PERF_CONTROLLER, TEST_DOMAIN_COUNT - 1U);

    status =
        mod_perf_controller_element_init(element_id, sub_element_count, NULL);

    TEST_ASSERT_EQUAL(status, FWK_E_PARAM);
}

void test_element_init_invalid_data(void)
{
    int status;
    unsigned int sub_element_count = MAX_LIMITER_PER_DOMAIN;
    fwk_id_t element_id = FWK_ID_ELEMENT_INIT(
        FWK_MODULE_IDX_PERF_CONTROLLER, TEST_DOMAIN_COUNT - 1U);

    status =
        mod_perf_controller_element_init(element_id, sub_element_count, NULL);

    TEST_ASSERT_EQUAL(status, FWK_E_PARAM);
}

void test_element_init_success(void)
{
    int status;
    unsigned int domain_idx;
    fwk_id_t domain_id;
    unsigned int sub_element_count;
    struct mod_perf_controller_domain_ctx *domain_ctx;
    uintptr_t allocated_memory_ptr = 12345U;

    for (domain_idx = 0U; domain_idx < TEST_DOMAIN_COUNT - 1U; domain_idx++) {
        domain_id = FWK_ID_ELEMENT(FWK_MODULE_IDX_PERF_CONTROLLER, domain_idx);

        domain_ctx = &perf_controller_ctx.domain_ctx_table[domain_idx];

        sub_element_count = 1U + (domain_idx % MAX_LIMITER_PER_DOMAIN);
        allocated_memory_ptr++;

        fwk_mm_calloc_ExpectAndReturn(
            sub_element_count,
            sizeof(struct mod_perf_controller_limiter_ctx),
            (void *)allocated_memory_ptr);

        status = mod_perf_controller_element_init(
            domain_id, sub_element_count, (void *)&domain_config[domain_idx]);

        TEST_ASSERT_EQUAL(status, FWK_SUCCESS);
        TEST_ASSERT_EQUAL_PTR(
            domain_ctx->limiter_ctx_table, allocated_memory_ptr);
        TEST_ASSERT_EQUAL_PTR(domain_ctx->config, &domain_config[domain_idx]);
    }
}

void test_bind_round_1(void)
{
    int status;
    unsigned int round = 1U;
    fwk_id_t id = FWK_ID_ELEMENT_INIT(FWK_MODULE_IDX_PERF_CONTROLLER, 0U);

    status = mod_perf_controller_bind(id, round);

    TEST_ASSERT_EQUAL(status, FWK_SUCCESS);
}

void test_bind_type_module(void)
{
    int status;
    unsigned int round = 0U;
    fwk_id_t id = FWK_ID_MODULE_INIT(FWK_MODULE_IDX_PERF_CONTROLLER);

    status = mod_perf_controller_bind(id, round);

    TEST_ASSERT_EQUAL(status, FWK_SUCCESS);
}

void test_bind_perf_drv_bind_error(void)
{
    int status;
    struct mod_perf_controller_domain_ctx *domain_ctx;
    unsigned int round = 0U;
    fwk_id_t id = FWK_ID_ELEMENT_INIT(FWK_MODULE_IDX_PERF_CONTROLLER, 0U);

    domain_ctx = &perf_controller_ctx.domain_ctx_table[0];
    domain_ctx->config =
        (struct mod_perf_controller_domain_config *)domain_config[0].data;

    fwk_module_bind_ExpectAndReturn(
        domain_ctx->config->performance_driver_id,
        domain_ctx->config->performance_driver_api_id,
        &domain_ctx->perf_driver_api,
        FWK_E_RANGE);

    status = mod_perf_controller_bind(id, round);

    TEST_ASSERT_EQUAL(status, FWK_E_RANGE);
}

void test_bind_success(void)
{
    int status;
    struct mod_perf_controller_domain_ctx *domain_ctx;
    unsigned int round = 0U;
    fwk_id_t id = FWK_ID_ELEMENT_INIT(FWK_MODULE_IDX_PERF_CONTROLLER, 0U);

    domain_ctx = &perf_controller_ctx.domain_ctx_table[0];
    domain_ctx->config =
        (struct mod_perf_controller_domain_config *)domain_config[0].data;

    fwk_module_bind_ExpectAndReturn(
        domain_ctx->config->performance_driver_id,
        domain_ctx->config->performance_driver_api_id,
        &domain_ctx->perf_driver_api,
        FWK_SUCCESS);

    fwk_module_bind_ExpectAndReturn(
        domain_ctx->config->power_model_id,
        domain_ctx->config->power_model_api_id,
        &domain_ctx->power_model_api,
        FWK_SUCCESS);

    status = mod_perf_controller_bind(id, round);

    TEST_ASSERT_EQUAL(status, FWK_SUCCESS);
}

void test_process_bind_request_wrong_module(void)
{
    int status;
    fwk_id_t source_id;
    fwk_id_t target_id;
    fwk_id_t api_id;
    const void *api;

    api_id =
        FWK_ID_API(FWK_MODULE_IDX_COUNT, MOD_PERF_CONTROLLER_API_COUNT - 1U);

    status = mod_perf_controller_process_bind_request(
        source_id, target_id, api_id, &api);

    TEST_ASSERT_EQUAL(status, FWK_E_PARAM);
}

void test_process_bind_request_wrong_api(void)
{
    int status;
    fwk_id_t source_id;
    fwk_id_t target_id;
    fwk_id_t api_id;
    const void *api;

    api_id = FWK_ID_API(
        FWK_MODULE_IDX_PERF_CONTROLLER, MOD_PERF_CONTROLLER_API_COUNT);

    status = mod_perf_controller_process_bind_request(
        source_id, target_id, api_id, &api);

    TEST_ASSERT_EQUAL(status, FWK_E_PARAM);
}

void test_process_bind_request_success(void)
{
    int status;
    fwk_id_t source_id;
    fwk_id_t target_id;
    fwk_id_t api_id;
    const void *api;
    unsigned int api_idx;

    const void *module_apis[MOD_PERF_CONTROLLER_API_COUNT] = {
        [MOD_PERF_CONTROLLER_DOMAIN_PERF_API] = &perf_api,
        [MOD_PERF_CONTROLLER_LIMITER_POWER_API] = &power_api,
        [MOD_PERF_CONTROLLER_APPLY_PERFORMANCE_GRANTED_API] =
            &apply_performance_granted_api,
    };

    for (api_idx = 0U; api_idx < MOD_PERF_CONTROLLER_API_COUNT; api_idx++) {
        api_id = FWK_ID_API(FWK_MODULE_IDX_PERF_CONTROLLER, api_idx);

        status = mod_perf_controller_process_bind_request(
            source_id, target_id, api_id, &api);

        TEST_ASSERT_EQUAL_PTR(api, module_apis[api_idx]);
        TEST_ASSERT_EQUAL(status, FWK_SUCCESS);
    }
}

int perf_controller_fwk_test_main(void)
{
    UNITY_BEGIN();

    RUN_TEST(test_init_invalid_element_count);
    RUN_TEST(test_init_success);
    RUN_TEST(test_element_init_invalid_sub_element_count);
    RUN_TEST(test_element_init_invalid_data);
    RUN_TEST(test_element_init_success);
    RUN_TEST(test_bind_round_1);
    RUN_TEST(test_bind_type_module);
    RUN_TEST(test_bind_perf_drv_bind_error);
    RUN_TEST(test_bind_success);
    RUN_TEST(test_process_bind_request_wrong_module);
    RUN_TEST(test_process_bind_request_wrong_api);
    RUN_TEST(test_process_bind_request_success);
    return UNITY_END();
}

#if !defined(TEST_ON_TAREGT)
int main(void)
{
    return perf_controller_fwk_test_main();
}
#endif
