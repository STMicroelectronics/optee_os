/*
 * Arm SCP/MCP Software
 * Copyright (c) 2024-2025, Arm Limited and Contributors. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 * Unit test for mod_perf_controller.c
 */

#include "scp_unity.h"
#include "unity.h"

#include <Mockfwk_module.h>
#include <Mockmod_perf_controller_extra.h>
#include <config_mod_perf_controller.h>
#include <internal/perf_controller.h>

#include <mod_perf_controller.h>

#include <fwk_module_idx.h>

#include <stdlib.h>

#include UNIT_TEST_SRC

static struct mod_perf_controller_domain_ctx
    test_domain_ctx_table[TEST_DOMAIN_COUNT];
static struct mod_perf_controller_limiter_ctx
    test_limiter_ctx_table[TEST_DOMAIN_COUNT][MAX_LIMITER_PER_DOMAIN];

static struct mod_perf_controller_drv_api perf_driver_api = {
    .set_performance_level = driver_set_performance_level,
};

static struct mod_perf_controller_power_model_api power_model_api = {
    .power_to_performance = power_to_performance,
};

void setUp(void)
{
    unsigned int domain_idx;
    struct mod_perf_controller_domain_ctx *domain_ctx;

    perf_controller_ctx.domain_ctx_table = test_domain_ctx_table;
    perf_controller_ctx.domain_count = TEST_DOMAIN_COUNT;

    for (domain_idx = 0U; domain_idx < TEST_DOMAIN_COUNT; domain_idx++) {
        domain_ctx = &perf_controller_ctx.domain_ctx_table[domain_idx];
        domain_ctx->limiter_ctx_table = test_limiter_ctx_table[domain_idx];
        domain_ctx->perf_driver_api = &perf_driver_api;
        domain_ctx->power_model_api = &power_model_api;
        domain_ctx->config = (struct mod_perf_controller_domain_config *)
                                 domain_config[domain_idx]
                                     .data;
        domain_ctx->limiter_count = domain_config[domain_idx].sub_element_count;
    }

    internal_api.get_limiters_min_power_limit =
        get_limiters_min_power_limit_stub;
    internal_api.domain_apply_performance_granted =
        domain_apply_performance_granted_stub;
}

void tearDown(void)
{
    Mockmod_perf_controller_extra_Verify();
    Mockmod_perf_controller_extra_Destroy();
}

/*!
 * \brief Helper function to compare two values.
 *
 * \details returns -1 when a < b,
 *          returns 1 when a >b,
 *          return 0 when a = b.
 */
int helper_comp(const void *a, const void *b)
{
    return (*(int *)a > *(int *)b) - (*(int *)a < *(int *)b);
}

void test_set_performance_level_within_limits(void)
{
    int status;
    unsigned int domain_idx;
    fwk_id_t domain_id;
    struct mod_perf_controller_domain_ctx *domain_ctx;
    uintptr_t cookie;
    uint32_t performance_level;

    for (domain_idx = 0U; domain_idx < TEST_DOMAIN_COUNT; domain_idx++) {
        domain_id = FWK_ID_ELEMENT(FWK_MODULE_IDX_PERF_CONTROLLER, domain_idx);
        domain_ctx = &perf_controller_ctx.domain_ctx_table[domain_idx];

        performance_level = 10U;
        domain_ctx->performance_limit = performance_level;

        cookie = 15U;
        driver_set_performance_level_ExpectAndReturn(
            domain_ctx->config->performance_driver_id,
            cookie,
            performance_level,
            FWK_SUCCESS);

        status = mod_perf_controller_set_performance_level(
            domain_id, cookie, performance_level);

        TEST_ASSERT_EQUAL(
            domain_ctx->performance_request_details.level, performance_level);
        TEST_ASSERT_EQUAL(domain_ctx->performance_request_details.cookie, 0U);
        TEST_ASSERT_EQUAL(status, FWK_SUCCESS);
    }
}

void test_set_performance_level_out_of_limits(void)
{
    int status;
    unsigned int domain_idx;
    fwk_id_t domain_id;
    struct mod_perf_controller_domain_ctx *domain_ctx;
    uintptr_t cookie;
    uint32_t performance_level;

    for (domain_idx = 0U; domain_idx < TEST_DOMAIN_COUNT; domain_idx++) {
        domain_id = FWK_ID_ELEMENT(FWK_MODULE_IDX_PERF_CONTROLLER, domain_idx);
        domain_ctx = &perf_controller_ctx.domain_ctx_table[domain_idx];

        domain_ctx->performance_limit = 10U;
        performance_level = domain_ctx->performance_limit + 1U;

        cookie = 1U;
        status = mod_perf_controller_set_performance_level(
            domain_id, cookie, performance_level);

        TEST_ASSERT_EQUAL(
            domain_ctx->performance_request_details.level, performance_level);
        TEST_ASSERT_EQUAL(
            domain_ctx->performance_request_details.cookie, cookie);
        TEST_ASSERT_EQUAL(status, FWK_PENDING);
    }
}

void test_set_power_limit_success(void)
{
    int status;
    fwk_id_t limiter_id;
    struct mod_perf_controller_limiter_ctx *limiter_ctx;
    struct mod_perf_controller_domain_ctx *domain_ctx;
    unsigned int domain_idx = TEST_DOMAIN_COUNT - 1U;
    unsigned int limiter_idx = MAX_LIMITER_PER_DOMAIN - 1U;
    uint32_t power_limit;

    for (domain_idx = 0U; domain_idx < TEST_DOMAIN_COUNT - 1U; domain_idx++) {
        domain_ctx = &perf_controller_ctx.domain_ctx_table[domain_idx];
        for (limiter_idx = 0U; limiter_idx < domain_ctx->limiter_count;
             limiter_idx++) {
            fwk_module_is_valid_sub_element_id_ExpectAnyArgsAndReturn(true);
            limiter_id = FWK_ID_SUB_ELEMENT(
                FWK_MODULE_IDX_PERF_CONTROLLER, domain_idx, limiter_idx);
            limiter_ctx = &perf_controller_ctx.domain_ctx_table[domain_idx]
                               .limiter_ctx_table[limiter_idx];

            power_limit = 20U;
            status =
                mod_perf_controller_set_power_limit(limiter_id, power_limit);

            TEST_ASSERT_EQUAL(limiter_ctx->power_limit, power_limit);
            TEST_ASSERT_EQUAL(status, FWK_SUCCESS);
        }
    }
}

void test_get_limiters_min_power_limit(void)
{
    uint32_t min_power_limit;
    unsigned int limiter_idx;
    unsigned int domain_idx;
    struct mod_perf_controller_domain_ctx *domain_ctx;
    uint32_t limiter_power_limit_test_values[MAX_LIMITER_PER_DOMAIN] = {
        100U, 300U, 200U, 10U
    };

    for (domain_idx = 0U; domain_idx < TEST_DOMAIN_COUNT - 1U; domain_idx++) {
        domain_ctx = &perf_controller_ctx.domain_ctx_table[domain_idx];

        for (limiter_idx = 0u; limiter_idx < domain_ctx->limiter_count;
             limiter_idx++) {
            domain_ctx->limiter_ctx_table[limiter_idx].power_limit =
                limiter_power_limit_test_values[limiter_idx];
        }

        min_power_limit = get_limiters_min_power_limit(domain_ctx);

        /*
            Using sorting to determine the minimum. qsort is used as it is a
            standard function that would make the test easier. The heavy lifting
            still needs to be done on the implementation side.
        */

        qsort(
            limiter_power_limit_test_values,
            domain_ctx->limiter_count,
            sizeof(limiter_power_limit_test_values[0]),
            helper_comp);

        TEST_ASSERT_EQUAL(limiter_power_limit_test_values[0], min_power_limit);
    }
}

void test_controller_domain_apply_performance_granted_within_limits(void)
{
    int status;
    unsigned int domain_idx;
    struct mod_perf_controller_domain_ctx *domain_ctx;
    uint32_t min_power_limit;
    uint32_t performance_limit;
    uint32_t *requested_performance;
    uintptr_t *cookie;

    for (domain_idx = 0U; domain_idx < TEST_DOMAIN_COUNT - 1U; domain_idx++) {
        domain_ctx = &perf_controller_ctx.domain_ctx_table[domain_idx];
        requested_performance = &domain_ctx->performance_request_details.level;
        cookie = &domain_ctx->performance_request_details.cookie;

        min_power_limit = 500U;
        performance_limit = 700U;

        get_limiters_min_power_limit_stub_ExpectAndReturn(
            domain_ctx, min_power_limit);

        power_to_performance_ExpectAndReturn(
            domain_ctx->config->power_model_id,
            min_power_limit,
            NULL,
            FWK_SUCCESS);

        power_to_performance_IgnoreArg_performance_level();

        power_to_performance_ReturnThruPtr_performance_level(
            &performance_limit);

        *requested_performance = performance_limit;
        *cookie = 2U;

        driver_set_performance_level_ExpectAndReturn(
            domain_ctx->config->performance_driver_id,
            *cookie,
            *requested_performance,
            FWK_SUCCESS);

        status = domain_apply_performance_granted(domain_ctx);

        TEST_ASSERT_EQUAL(domain_ctx->performance_limit, performance_limit);
        TEST_ASSERT_EQUAL(status, FWK_SUCCESS);
    }
}

void test_controller_domain_apply_performance_granted_out_of_limits(void)
{
    int status;
    unsigned int domain_idx;
    struct mod_perf_controller_domain_ctx *domain_ctx;
    uint32_t min_power_limit;
    uint32_t performance_limit;
    uint32_t *requested_performance;
    uintptr_t *cookie;

    for (domain_idx = 0U; domain_idx < TEST_DOMAIN_COUNT - 1U; domain_idx++) {
        domain_ctx = &perf_controller_ctx.domain_ctx_table[domain_idx];
        requested_performance = &domain_ctx->performance_request_details.level;
        cookie = &domain_ctx->performance_request_details.cookie;

        min_power_limit = 800U;
        performance_limit = 991U;

        get_limiters_min_power_limit_stub_ExpectAndReturn(
            domain_ctx, min_power_limit);

        power_to_performance_ExpectAndReturn(
            domain_ctx->config->power_model_id,
            min_power_limit,
            NULL,
            FWK_SUCCESS);

        power_to_performance_IgnoreArg_performance_level();

        power_to_performance_ReturnThruPtr_performance_level(
            &performance_limit);

        *requested_performance = performance_limit + 1U;
        *cookie = 3U;

        driver_set_performance_level_ExpectAndReturn(
            domain_ctx->config->performance_driver_id,
            0U, /* No cookie */
            performance_limit,
            FWK_SUCCESS);

        status = domain_apply_performance_granted(domain_ctx);

        TEST_ASSERT_EQUAL(status, FWK_SUCCESS);
    }
}

void test_controller_domain_apply_performance_granted_success(void)
{
    int status;
    unsigned int domain_idx;
    struct mod_perf_controller_domain_ctx *domain_ctx;

    for (domain_idx = 0U; domain_idx < TEST_DOMAIN_COUNT; domain_idx++) {
        domain_ctx = &perf_controller_ctx.domain_ctx_table[domain_idx];
        domain_apply_performance_granted_stub_ExpectAndReturn(
            domain_ctx, FWK_SUCCESS);
    }

    status = mod_perf_controller_apply_performance_granted();

    TEST_ASSERT_EQUAL(status, FWK_SUCCESS);
}

int perf_controller_test_main(void)
{
    UNITY_BEGIN();

    RUN_TEST(test_set_performance_level_within_limits);
    RUN_TEST(test_set_performance_level_out_of_limits);
    RUN_TEST(test_set_power_limit_success);
    RUN_TEST(test_get_limiters_min_power_limit);
    RUN_TEST(test_controller_domain_apply_performance_granted_within_limits);
    RUN_TEST(test_controller_domain_apply_performance_granted_out_of_limits);
    RUN_TEST(test_controller_domain_apply_performance_granted_success);

    return UNITY_END();
}

#if !defined(TEST_ON_TARGET)
int main(void)
{
    return perf_controller_test_main();
}
#endif
