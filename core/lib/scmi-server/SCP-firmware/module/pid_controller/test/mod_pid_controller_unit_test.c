/*
 * Arm SCP/MCP Software
 * Copyright (c) 2024, Arm Limited and Contributors. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 * Description:
 *      PID Controller unit tests.
 */

#include "scp_unity.h"
#include "unity.h"

#include <Mockfwk_id.h>
#include <Mockfwk_mm.h>
#include <Mockfwk_module.h>

#include <mod_pid_controller.h>

#include <fwk_element.h>

#include UNIT_TEST_SRC

enum mod_pid_elem_id {
    PID_CONTROLLER_FAKE_INDEX_0,
    PID_CONTROLLER_FAKE_INDEX_COUNT,
};

static void pid_controller_init_one_element(
    struct mod_pid_controller_elem_ctx *mock_elem_ctx,
    struct mod_pid_controller_elem_config *mock_config)
{
    mod_ctx.element_count = PID_CONTROLLER_FAKE_INDEX_COUNT;
    mod_ctx.elem_ctx_table = mock_elem_ctx;
    mock_elem_ctx->config = mock_config;
}

void setUp(void)
{
}

void tearDown(void)
{
}

static void test_pid_controller_init_success(void)
{
    struct mod_pid_controller_elem_ctx *return_table =
        (struct mod_pid_controller_elem_ctx *)0x7A7A7A7A;
    unsigned int element_count = 1;
    int status;

    fwk_mm_calloc_ExpectAndReturn(
        element_count,
        sizeof(struct mod_pid_controller_elem_ctx),
        (void *)return_table);

    status =
        pid_controller_init(fwk_module_id_pid_controller, element_count, NULL);

    TEST_ASSERT_EQUAL(FWK_SUCCESS, status);
    TEST_ASSERT_EQUAL_PTR(return_table, mod_ctx.elem_ctx_table);
    TEST_ASSERT_EQUAL(element_count, mod_ctx.element_count);
}

static void test_pid_controller_element_init_success(void)
{
    fwk_id_t element_id = FWK_ID_ELEMENT_INIT(
        FWK_MODULE_IDX_PID_CONTROLLER, PID_CONTROLLER_FAKE_INDEX_0);
    struct mod_pid_controller_elem_ctx mock_elem_ctx;
    struct mod_pid_controller_elem_config mock_config = { .set_point = 42 };
    unsigned int unused = 0;
    int status;

    pid_controller_init_one_element(&mock_elem_ctx, &mock_config);
    fwk_id_get_element_idx_ExpectAndReturn(
        element_id, PID_CONTROLLER_FAKE_INDEX_0);
    fwk_id_get_element_idx_ExpectAndReturn(
        element_id, PID_CONTROLLER_FAKE_INDEX_0);

    status = pid_controller_element_init(
        element_id, unused, (const void *)&mock_config);

    TEST_ASSERT_EQUAL(FWK_SUCCESS, status);
    TEST_ASSERT_EQUAL_PTR(&mock_config, mock_elem_ctx.config);
}

static void test_pid_controller_bind_request_api_control(void)
{
    fwk_id_t source_id = FWK_ID_ELEMENT_INIT(FWK_MODULE_IDX_FAKE_0, 0);
    fwk_id_t target_id = FWK_ID_ELEMENT_INIT(
        FWK_MODULE_IDX_PID_CONTROLLER, PID_CONTROLLER_FAKE_INDEX_0);
    fwk_id_t api_id = FWK_ID_API_INIT(
        FWK_MODULE_IDX_PID_CONTROLLER, MOD_PID_CONTROLLER_API_IDX_CONTROL);
    const void *api = NULL;
    int status;

    fwk_id_get_api_idx_ExpectAndReturn(
        api_id, MOD_PID_CONTROLLER_API_IDX_CONTROL);

    status = pid_controller_bind_request(source_id, target_id, api_id, &api);

    TEST_ASSERT_EQUAL(FWK_SUCCESS, status);
    TEST_ASSERT_EQUAL_PTR(&pid_controller_api, api);
}

static void test_pid_controller_bind_request_api_invalid_id(void)
{
    fwk_id_t source_id = FWK_ID_ELEMENT_INIT(FWK_MODULE_IDX_FAKE_0, 0);
    fwk_id_t target_id = FWK_ID_ELEMENT(
        FWK_MODULE_IDX_PID_CONTROLLER, PID_CONTROLLER_FAKE_INDEX_0);
    fwk_id_t api_id = FWK_ID_API(
        FWK_MODULE_IDX_PID_CONTROLLER, MOD_PID_CONTROLLER_API_IDX_CONTROL);
    const void *api = NULL;
    int status;

    fwk_id_get_api_idx_ExpectAndReturn(
        api_id, MOD_PID_CONTROLLER_API_IDX_COUNT);

    status = pid_controller_bind_request(source_id, target_id, api_id, &api);

    TEST_ASSERT_EQUAL(FWK_E_PARAM, status);
    TEST_ASSERT_NULL(api);
}

static void test_get_elem_ctx_valid_id(void)
{
    struct mod_pid_controller_elem_ctx mock_elem_ctx;
    fwk_id_t mock_id = FWK_ID_ELEMENT_INIT(
        FWK_MODULE_IDX_PID_CONTROLLER, PID_CONTROLLER_FAKE_INDEX_0);
    pid_controller_init_one_element(&mock_elem_ctx, NULL);

    fwk_id_get_element_idx_ExpectAndReturn(
        mock_id, PID_CONTROLLER_FAKE_INDEX_0);

    struct mod_pid_controller_elem_ctx *ctx = get_elem_ctx(mock_id);
    TEST_ASSERT_EQUAL_PTR(&mock_elem_ctx, ctx);
}

static void test_get_elem_ctx_invalid_id(void)
{
    mod_ctx.element_count = PID_CONTROLLER_FAKE_INDEX_COUNT;

    fwk_id_t mock_id = FWK_ID_ELEMENT_INIT(
        FWK_MODULE_IDX_PID_CONTROLLER, PID_CONTROLLER_FAKE_INDEX_COUNT);
    fwk_id_get_element_idx_ExpectAndReturn(
        mock_id, PID_CONTROLLER_FAKE_INDEX_COUNT);

    struct mod_pid_controller_elem_ctx *ctx = get_elem_ctx(mock_id);
    TEST_ASSERT_NULL(ctx);
}

static void test_reset_error_values(void)
{
    struct mod_pid_controller_elem_ctx mock_elem_ctx;

    mock_elem_ctx.error.integral = 1;
    mock_elem_ctx.error.derivative = 2;
    mock_elem_ctx.error.current = 3;
    mock_elem_ctx.error.previous = 4;

    reset_error_values(&mock_elem_ctx);

    TEST_ASSERT_EQUAL(0, mock_elem_ctx.error.integral);
    TEST_ASSERT_EQUAL(0, mock_elem_ctx.error.derivative);
    TEST_ASSERT_EQUAL(0, mock_elem_ctx.error.current);
    TEST_ASSERT_EQUAL(0, mock_elem_ctx.error.previous);
}

static void test_is_integral_error_overflow_no_overflow(void)
{
    struct mod_pid_controller_elem_ctx mock_elem_ctx;
    bool overflow;

    mock_elem_ctx.error.current = 5;
    mock_elem_ctx.error.integral = INT64_MAX - 6;

    overflow = is_integral_error_overflow(&mock_elem_ctx);

    TEST_ASSERT_FALSE(overflow);
}

static void test_is_integral_error_overflow(void)
{
    struct mod_pid_controller_elem_ctx mock_elem_ctx;
    bool overflow;

    mock_elem_ctx.error.current = 5;
    mock_elem_ctx.error.integral = INT64_MAX - 2;

    overflow = is_integral_error_overflow(&mock_elem_ctx);

    TEST_ASSERT_TRUE(overflow);

    mock_elem_ctx.error.current = -5;
    mock_elem_ctx.error.integral = INT64_MIN + 2;

    overflow = is_integral_error_overflow(&mock_elem_ctx);

    TEST_ASSERT_TRUE(overflow);
}

static void test_get_k_prop(void)
{
    struct mod_pid_controller_elem_ctx mock_elem_ctx;
    struct mod_pid_controller_elem_config mock_config;
    const struct mod_pid_controller_k_parameter *k_prop;

    pid_controller_init_one_element(&mock_elem_ctx, &mock_config);

    mock_elem_ctx.error.current = -1;
    k_prop = get_k_prop(&mock_elem_ctx);
    TEST_ASSERT_EQUAL_PTR(&mock_config.k.proportional_overshoot, k_prop);

    mock_elem_ctx.error.current = 1;
    k_prop = get_k_prop(&mock_elem_ctx);
    TEST_ASSERT_EQUAL_PTR(&mock_config.k.proportional_undershoot, k_prop);
}

static void test_pid_controller_calculate_output(void)
{
    struct mod_pid_controller_elem_ctx mock_elem_ctx;
    struct mod_pid_controller_elem_config mock_config;
    int64_t output;

    pid_controller_init_one_element(&mock_elem_ctx, &mock_config);

    mock_config.output.min = -100;
    mock_config.output.max = 100;
    memset(&mock_config.k, 0, sizeof(mock_config.k));
    mock_config.k.proportional_overshoot.numerator = 1;
    mock_config.k.proportional_undershoot.numerator = 1;
    mock_config.k.integral.numerator = 1;
    mock_config.k.derivative.numerator = 1;
    mock_elem_ctx.error.current = 1;
    mock_elem_ctx.error.integral = 1;
    mock_elem_ctx.error.derivative = 1;

    output = calculate_output(&mock_elem_ctx);
    TEST_ASSERT_EQUAL(3, output);
}

static void test_pid_controller_calculate_output_with_constraints(void)
{
    struct mod_pid_controller_elem_ctx mock_elem_ctx;
    struct mod_pid_controller_elem_config mock_config;
    int64_t output;

    pid_controller_init_one_element(&mock_elem_ctx, &mock_config);

    mock_config.output.min = -2;
    mock_config.output.max = 2;
    memset(&mock_config.k, 0, sizeof(mock_config.k));
    mock_config.k.proportional_overshoot.numerator = 1;
    mock_config.k.proportional_undershoot.numerator = 1;
    mock_config.k.integral.numerator = 1;
    mock_config.k.derivative.numerator = 1;
    mock_elem_ctx.error.current = 1;
    mock_elem_ctx.error.integral = 1;
    mock_elem_ctx.error.derivative = 1;

    output = calculate_output(&mock_elem_ctx);
    TEST_ASSERT_EQUAL(mock_config.output.max, output);

    mock_elem_ctx.error.current = -1;
    mock_elem_ctx.error.integral = -1;
    mock_elem_ctx.error.derivative = -1;

    output = calculate_output(&mock_elem_ctx);
    TEST_ASSERT_EQUAL(mock_config.output.min, output);
}

static void test_pid_controller_update_inactive_output(void)
{
    struct mod_pid_controller_elem_ctx mock_elem_ctx;
    struct mod_pid_controller_elem_config mock_config;
    int64_t input, output;
    int status;
    fwk_id_t mock_id = FWK_ID_ELEMENT_INIT(
        FWK_MODULE_IDX_PID_CONTROLLER, PID_CONTROLLER_FAKE_INDEX_0);

    pid_controller_init_one_element(&mock_elem_ctx, &mock_config);

    mock_config.switch_on_value = 50;
    mock_config.inactive_state_output = 20;
    input = mock_config.switch_on_value - 1;

    fwk_id_get_element_idx_ExpectAndReturn(
        mock_id, PID_CONTROLLER_FAKE_INDEX_0);

    status = pid_controller_update(mock_id, input, &output);

    TEST_ASSERT_EQUAL(FWK_SUCCESS, status);
    TEST_ASSERT_EQUAL(mock_config.inactive_state_output, output);
}

static void test_pid_controller_update_integral_overflow(void)
{
    struct mod_pid_controller_elem_ctx mock_elem_ctx;
    struct mod_pid_controller_elem_config mock_config;
    int64_t input, output;
    int status;
    fwk_id_t mock_id = FWK_ID_ELEMENT_INIT(
        FWK_MODULE_IDX_PID_CONTROLLER, PID_CONTROLLER_FAKE_INDEX_0);

    pid_controller_init_one_element(&mock_elem_ctx, &mock_config);

    mock_config.switch_on_value = 50;
    input = mock_config.switch_on_value + 1;
    mock_elem_ctx.set_point = input + 1;
    mock_elem_ctx.error.integral =
        INT64_MAX - (mock_elem_ctx.set_point - input) + 1;

    fwk_id_get_element_idx_ExpectAndReturn(
        mock_id, PID_CONTROLLER_FAKE_INDEX_0);

    status = pid_controller_update(mock_id, input, &output);

    TEST_ASSERT_EQUAL(FWK_E_DATA, status);
}

static void test_pid_controller_update_output_calculation(void)
{
    struct mod_pid_controller_elem_ctx mock_elem_ctx;
    struct mod_pid_controller_elem_config mock_config;
    int64_t input, output;
    int status;
    fwk_id_t mock_id = FWK_ID_ELEMENT_INIT(
        FWK_MODULE_IDX_PID_CONTROLLER, PID_CONTROLLER_FAKE_INDEX_0);

    pid_controller_init_one_element(&mock_elem_ctx, &mock_config);

    mock_config.output.min = -100;
    mock_config.output.max = 100;
    mock_config.integral_max = 100;
    mock_config.integral_cutoff = 100;
    mock_config.switch_on_value = 50;
    memset(&mock_config.k, 0, sizeof(mock_config.k));
    mock_config.k.proportional_overshoot.numerator = 1;
    mock_config.k.proportional_undershoot.numerator = 1;
    mock_config.k.integral.numerator = 1;
    mock_config.k.derivative.numerator = 1;
    input = mock_config.switch_on_value + 1;
    mock_elem_ctx.set_point = input + 1;

    mock_elem_ctx.error.integral = 0;
    mock_elem_ctx.error.previous = 0;

    fwk_id_get_element_idx_ExpectAndReturn(
        mock_id, PID_CONTROLLER_FAKE_INDEX_0);

    status = pid_controller_update(mock_id, input, &output);

    TEST_ASSERT_EQUAL(FWK_SUCCESS, status);
    TEST_ASSERT_EQUAL(3, output);
}

static void test_pid_controller_set_point_valid_input(void)
{
    struct mod_pid_controller_elem_ctx mock_elem_ctx;
    struct mod_pid_controller_elem_config mock_config;
    int64_t new_set_point;
    int status;
    fwk_id_t mock_id = FWK_ID_ELEMENT_INIT(
        FWK_MODULE_IDX_PID_CONTROLLER, PID_CONTROLLER_FAKE_INDEX_0);

    pid_controller_init_one_element(&mock_elem_ctx, &mock_config);
    new_set_point = mock_config.switch_on_value + 1;

    fwk_id_get_element_idx_ExpectAndReturn(
        mock_id, PID_CONTROLLER_FAKE_INDEX_0);

    status = pid_controller_set_point(mock_id, new_set_point);
    TEST_ASSERT_EQUAL(FWK_SUCCESS, status);
    TEST_ASSERT_EQUAL(new_set_point, mock_elem_ctx.set_point);
}

static void test_pid_controller_set_point_equal_to_switch_on_value(void)
{
    struct mod_pid_controller_elem_ctx mock_elem_ctx;
    struct mod_pid_controller_elem_config mock_config;
    int64_t new_set_point;
    int status;
    fwk_id_t mock_id = FWK_ID_ELEMENT_INIT(
        FWK_MODULE_IDX_PID_CONTROLLER, PID_CONTROLLER_FAKE_INDEX_0);

    pid_controller_init_one_element(&mock_elem_ctx, &mock_config);
    new_set_point = mock_elem_ctx.config->switch_on_value;

    fwk_id_get_element_idx_ExpectAndReturn(
        mock_id, PID_CONTROLLER_FAKE_INDEX_0);

    status = pid_controller_set_point(mock_id, new_set_point);
    TEST_ASSERT_EQUAL(FWK_SUCCESS, status);
    TEST_ASSERT_EQUAL(new_set_point, mock_elem_ctx.set_point);
}

static void test_pid_controller_set_point_below_to_switch_on_value(void)
{
    struct mod_pid_controller_elem_ctx mock_elem_ctx;
    struct mod_pid_controller_elem_config mock_config;
    int64_t new_set_point, old_set_point;
    int status;
    fwk_id_t mock_id = FWK_ID_ELEMENT_INIT(
        FWK_MODULE_IDX_PID_CONTROLLER, PID_CONTROLLER_FAKE_INDEX_0);

    pid_controller_init_one_element(&mock_elem_ctx, &mock_config);
    new_set_point = mock_elem_ctx.config->switch_on_value - 1;
    old_set_point = mock_elem_ctx.set_point;

    fwk_id_get_element_idx_ExpectAndReturn(
        mock_id, PID_CONTROLLER_FAKE_INDEX_0);

    status = pid_controller_set_point(mock_id, new_set_point);
    TEST_ASSERT_EQUAL(FWK_E_PARAM, status);
    TEST_ASSERT_EQUAL(old_set_point, mock_elem_ctx.set_point);
}

static void test_pid_controller_reset(void)
{
    struct mod_pid_controller_elem_ctx mock_elem_ctx;
    int status;
    fwk_id_t mock_id = FWK_ID_ELEMENT_INIT(
        FWK_MODULE_IDX_PID_CONTROLLER, PID_CONTROLLER_FAKE_INDEX_0);

    pid_controller_init_one_element(&mock_elem_ctx, NULL);
    mock_elem_ctx.error.integral = 1;
    mock_elem_ctx.error.derivative = 2;
    mock_elem_ctx.error.current = 3;
    mock_elem_ctx.error.previous = 4;

    fwk_id_get_element_idx_ExpectAndReturn(
        mock_id, PID_CONTROLLER_FAKE_INDEX_0);

    status = pid_controller_reset(mock_id);

    TEST_ASSERT_EQUAL(FWK_SUCCESS, status);
    TEST_ASSERT_EQUAL(0, mock_elem_ctx.error.integral);
    TEST_ASSERT_EQUAL(0, mock_elem_ctx.error.derivative);
    TEST_ASSERT_EQUAL(0, mock_elem_ctx.error.current);
    TEST_ASSERT_EQUAL(0, mock_elem_ctx.error.previous);
}

int pid_controller_test_main(void)
{
    UNITY_BEGIN();

    RUN_TEST(test_pid_controller_init_success);
    RUN_TEST(test_pid_controller_element_init_success);
    RUN_TEST(test_pid_controller_bind_request_api_control);
    RUN_TEST(test_pid_controller_bind_request_api_invalid_id);
    RUN_TEST(test_get_elem_ctx_valid_id);
    RUN_TEST(test_get_elem_ctx_invalid_id);
    RUN_TEST(test_reset_error_values);
    RUN_TEST(test_is_integral_error_overflow_no_overflow);
    RUN_TEST(test_is_integral_error_overflow);
    RUN_TEST(test_get_k_prop);
    RUN_TEST(test_pid_controller_calculate_output);
    RUN_TEST(test_pid_controller_calculate_output_with_constraints);
    RUN_TEST(test_pid_controller_update_inactive_output);
    RUN_TEST(test_pid_controller_update_integral_overflow);
    RUN_TEST(test_pid_controller_update_output_calculation);
    RUN_TEST(test_pid_controller_set_point_valid_input);
    RUN_TEST(test_pid_controller_set_point_equal_to_switch_on_value);
    RUN_TEST(test_pid_controller_set_point_below_to_switch_on_value);
    RUN_TEST(test_pid_controller_reset);

    return UNITY_END();
}

#if !defined(TEST_ON_TARGET)
int main(void)
{
    return pid_controller_test_main();
}
#endif
