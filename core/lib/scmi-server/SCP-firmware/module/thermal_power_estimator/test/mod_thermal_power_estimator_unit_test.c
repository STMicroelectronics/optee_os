/*
 * Arm SCP/MCP Software
 * Copyright (c) 2024, Arm Limited and Contributors. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include "scp_unity.h"
#include "unity.h"

#include <Mockfwk_id.h>
#include <Mockfwk_mm.h>
#include <Mockfwk_module.h>
#include <Mockmod_thermal_power_estimator_extra.h>
#include <internal/Mockfwk_core_internal.h>

#include UNIT_TEST_SRC

enum mod_thermal_power_estimator_elem_id {
    THERMAL_POWER_ESTIMATOR_FAKE_ELEM_IDX_0,
    THERMAL_POWER_ESTIMATOR_FAKE_ELEM_IDX_COUNT,
};

static void thermal_power_estimator_init_one_element(
    struct mod_thermal_power_estimator_dev_ctx *mock_dev_ctx,
    struct mod_thermal_power_estimator_dev_config *mock_config)
{
    mod_ctx.dev_ctx_count = THERMAL_POWER_ESTIMATOR_FAKE_ELEM_IDX_COUNT;
    mod_ctx.dev_ctx_table = mock_dev_ctx;
    mock_dev_ctx->config = mock_config;
}

struct mod_sensor_api mock_sensor_api = {
    .get_data = mod_sensor_get_data,
};

struct mod_pid_controller_api mock_pid_controller_api = {
    .update = mod_pid_controller_update,
};

void setUp(void)
{
}

void tearDown(void)
{
}

/* Test thermal power estimator module initialization */
static void test_thermal_power_estimator_init_success(void)
{
    struct mod_thermal_power_estimator_dev_ctx
        return_table[THERMAL_POWER_ESTIMATOR_FAKE_ELEM_IDX_COUNT];
    unsigned int element_count = THERMAL_POWER_ESTIMATOR_FAKE_ELEM_IDX_COUNT;
    int status;

    fwk_mm_calloc_ExpectAndReturn(
        element_count,
        sizeof(struct mod_thermal_power_estimator_dev_ctx),
        (void *)&return_table);

    status = thermal_power_estimator_init(
        fwk_module_id_thermal_power_estimator, element_count, NULL);

    TEST_ASSERT_EQUAL(FWK_SUCCESS, status);
    TEST_ASSERT_EQUAL_PTR(&return_table, mod_ctx.dev_ctx_table);
    TEST_ASSERT_EQUAL(element_count, mod_ctx.dev_ctx_count);
}

static void test_thermal_power_estimator_dev_init_success(void)
{
    fwk_id_t element_id = FWK_ID_ELEMENT_INIT(
        FWK_MODULE_IDX_THERMAL_POWER_ESTIMATOR,
        THERMAL_POWER_ESTIMATOR_FAKE_ELEM_IDX_0);
    struct mod_thermal_power_estimator_dev_ctx mock_dev_ctx;
    struct mod_thermal_power_estimator_dev_config mock_config = { .tdp = 100 };
    unsigned int unused = 0;
    int status;

    thermal_power_estimator_init_one_element(&mock_dev_ctx, &mock_config);
    fwk_id_get_element_idx_ExpectAndReturn(
        element_id, THERMAL_POWER_ESTIMATOR_FAKE_ELEM_IDX_0);

    status = thermal_power_estimator_dev_init(
        element_id, unused, (const void *)&mock_config);

    TEST_ASSERT_EQUAL(FWK_SUCCESS, status);
    TEST_ASSERT_EQUAL_PTR(&mock_config, mock_dev_ctx.config);
}

static void test_thermal_power_estimator_bind_success(void)
{
    fwk_id_t element_id = FWK_ID_ELEMENT_INIT(
        FWK_MODULE_IDX_THERMAL_POWER_ESTIMATOR,
        THERMAL_POWER_ESTIMATOR_FAKE_ELEM_IDX_0);
    struct mod_thermal_power_estimator_dev_ctx mock_dev_ctx;
    struct mod_thermal_power_estimator_dev_config mock_config = {
        .sensor_id = FWK_ID_ELEMENT_INIT(FWK_MODULE_IDX_SENSOR, 0),
        .sensor_api_id = FWK_ID_API_INIT(FWK_MODULE_IDX_SENSOR, 0),
        .pid_controller_id =
            FWK_ID_ELEMENT_INIT(FWK_MODULE_IDX_PID_CONTROLLER, 0),
        .pid_controller_api_id =
            FWK_ID_API_INIT(FWK_MODULE_IDX_PID_CONTROLLER, 0)
    };
    unsigned int round;
    int status;

    round = 1;
    status = thermal_power_estimator_bind(element_id, round);
    TEST_ASSERT_EQUAL(FWK_SUCCESS, status);

    round = 0;
    fwk_id_is_type_ExpectAnyArgsAndReturn(true);
    status = thermal_power_estimator_bind(element_id, round);
    TEST_ASSERT_EQUAL(FWK_SUCCESS, status);

    round = 0;
    thermal_power_estimator_init_one_element(&mock_dev_ctx, &mock_config);
    fwk_id_is_type_ExpectAnyArgsAndReturn(false);
    fwk_id_get_element_idx_ExpectAndReturn(
        element_id, THERMAL_POWER_ESTIMATOR_FAKE_ELEM_IDX_0);

    fwk_module_bind_ExpectAndReturn(
        mock_config.sensor_id,
        mock_config.sensor_api_id,
        &mock_dev_ctx.sensor_api,
        FWK_SUCCESS);
    fwk_module_bind_ExpectAndReturn(
        mock_config.pid_controller_id,
        mock_config.pid_controller_api_id,
        &mock_dev_ctx.pid_ctrl_api,
        FWK_SUCCESS);

    status = thermal_power_estimator_bind(element_id, round);
    TEST_ASSERT_EQUAL(FWK_SUCCESS, status);
}

static void test_thermal_power_estimator_bind_sensor_bind_fail(void)
{
    fwk_id_t element_id = FWK_ID_ELEMENT_INIT(
        FWK_MODULE_IDX_THERMAL_POWER_ESTIMATOR,
        THERMAL_POWER_ESTIMATOR_FAKE_ELEM_IDX_0);
    struct mod_thermal_power_estimator_dev_ctx mock_dev_ctx;
    struct mod_thermal_power_estimator_dev_config mock_config;
    unsigned int round;
    int status;

    round = 0;
    thermal_power_estimator_init_one_element(&mock_dev_ctx, &mock_config);
    fwk_id_get_element_idx_ExpectAndReturn(
        element_id, THERMAL_POWER_ESTIMATOR_FAKE_ELEM_IDX_0);
    fwk_id_is_type_ExpectAnyArgsAndReturn(false);

    fwk_module_bind_ExpectAnyArgsAndReturn(FWK_E_HANDLER);
    status = thermal_power_estimator_bind(element_id, round);
    TEST_ASSERT_EQUAL(FWK_E_PANIC, status);
}

static void test_thermal_power_estimator_process_bind_request_success(void)
{
    fwk_id_t source_id = FWK_ID_ELEMENT_INIT(FWK_MODULE_IDX_FAKE, 0);
    fwk_id_t target_id = FWK_ID_ELEMENT_INIT(
        FWK_MODULE_IDX_THERMAL_POWER_ESTIMATOR,
        THERMAL_POWER_ESTIMATOR_FAKE_ELEM_IDX_0);
    fwk_id_t api_id = FWK_ID_API_INIT(
        FWK_MODULE_IDX_THERMAL_POWER_ESTIMATOR,
        MOD_THERMAL_POWER_ESTIMATOR_API_IDX_POWER_MANAGEMENT);
    const void *api = NULL;
    int status;

    fwk_id_get_api_idx_ExpectAndReturn(
        api_id, MOD_THERMAL_POWER_ESTIMATOR_API_IDX_POWER_MANAGEMENT);

    status = thermal_power_estimator_process_bind_request(
        source_id, target_id, api_id, &api);

    TEST_ASSERT_EQUAL(FWK_SUCCESS, status);
    TEST_ASSERT_EQUAL_PTR(&thermal_power_estimator_api, api);
}

static void test_thermal_power_estimator_process_bind_request_invalid_api(void)
{
    fwk_id_t source_id = FWK_ID_ELEMENT_INIT(FWK_MODULE_IDX_FAKE, 0);
    fwk_id_t target_id = FWK_ID_ELEMENT_INIT(
        FWK_MODULE_IDX_THERMAL_POWER_ESTIMATOR,
        THERMAL_POWER_ESTIMATOR_FAKE_ELEM_IDX_0);
    fwk_id_t api_id = FWK_ID_API_INIT(
        FWK_MODULE_IDX_THERMAL_POWER_ESTIMATOR,
        MOD_THERMAL_POWER_ESTIMATOR_API_IDX_COUNT);
    const void *api = NULL;
    int status;

    fwk_id_get_api_idx_ExpectAndReturn(
        api_id, MOD_THERMAL_POWER_ESTIMATOR_API_IDX_COUNT);

    status = thermal_power_estimator_process_bind_request(
        source_id, target_id, api_id, &api);

    TEST_ASSERT_EQUAL(FWK_E_PARAM, status);
    TEST_ASSERT_NULL(api);
}

static void test_read_temperature_success(void)
{
    struct mod_thermal_power_estimator_dev_ctx mock_dev_ctx = {
        .sensor_api = &mock_sensor_api,
    };
    struct mod_thermal_power_estimator_dev_config mock_config = {
        .sensor_id = FWK_ID_ELEMENT_INIT(FWK_MODULE_IDX_SENSOR, 0),
    };
    int status;

    thermal_power_estimator_init_one_element(&mock_dev_ctx, &mock_config);

    mod_sensor_get_data_ExpectAndReturn(
        mock_config.sensor_id, &mock_dev_ctx.sensor_data, FWK_SUCCESS);

    status = read_temperature(&mock_dev_ctx);

    TEST_ASSERT_EQUAL(FWK_SUCCESS, status);
}

static void test_read_temperature_fail_pending(void)
{
    struct mod_thermal_power_estimator_dev_ctx mock_dev_ctx = {
        .sensor_api = &mock_sensor_api,
    };
    struct mod_thermal_power_estimator_dev_config mock_config = {
        .sensor_id = FWK_ID_ELEMENT_INIT(FWK_MODULE_IDX_SENSOR, 0),
    };
    int status;

    thermal_power_estimator_init_one_element(&mock_dev_ctx, &mock_config);

    mod_sensor_get_data_ExpectAndReturn(
        mock_config.sensor_id, &mock_dev_ctx.sensor_data, FWK_PENDING);

    status = read_temperature(&mock_dev_ctx);

    TEST_ASSERT_EQUAL(FWK_E_SUPPORT, status);
}

static void test_read_temperature_fail_generic(void)
{
    struct mod_thermal_power_estimator_dev_ctx mock_dev_ctx = {
        .sensor_api = &mock_sensor_api,
    };
    struct mod_thermal_power_estimator_dev_config mock_config = {
        .sensor_id = FWK_ID_ELEMENT_INIT(FWK_MODULE_IDX_SENSOR, 0),
    };
    int status;

    thermal_power_estimator_init_one_element(&mock_dev_ctx, &mock_config);

    mod_sensor_get_data_ExpectAndReturn(
        mock_config.sensor_id, &mock_dev_ctx.sensor_data, FWK_E_SUPPORT);

    status = read_temperature(&mock_dev_ctx);

    TEST_ASSERT_EQUAL(FWK_E_SUPPORT, status);
}

static void test_thermal_allocatable_power_calculate_positive_output(void)
{
    struct mod_thermal_power_estimator_dev_ctx mock_dev_ctx = { 0 };
    struct mod_thermal_power_estimator_dev_config mock_config = {
        .tdp = 100,
    };
    int64_t pid_output = 50;
    uint32_t allocatable_power;
    int status;

    thermal_power_estimator_init_one_element(&mock_dev_ctx, &mock_config);

    status = thermal_allocatable_power_calculate(
        &mock_dev_ctx, pid_output, &allocatable_power);

    TEST_ASSERT_EQUAL(FWK_SUCCESS, status);
    TEST_ASSERT_EQUAL(150, allocatable_power);
}

static void test_thermal_allocatable_power_calculate_negative_output(void)
{
    struct mod_thermal_power_estimator_dev_ctx mock_dev_ctx = { 0 };
    struct mod_thermal_power_estimator_dev_config mock_config = {
        .tdp = 100,
    };
    int64_t pid_output = -150;
    uint32_t allocatable_power;
    int status;

    thermal_power_estimator_init_one_element(&mock_dev_ctx, &mock_config);

    status = thermal_allocatable_power_calculate(
        &mock_dev_ctx, pid_output, &allocatable_power);

    TEST_ASSERT_EQUAL(FWK_SUCCESS, status);
    TEST_ASSERT_EQUAL(0, allocatable_power);
}

static void test_thermal_allocatable_power_calculate_zero_output(void)
{
    struct mod_thermal_power_estimator_dev_ctx mock_dev_ctx = { 0 };
    struct mod_thermal_power_estimator_dev_config mock_config = {
        .tdp = 100,
    };
    int64_t pid_output = -100;
    uint32_t allocatable_power;
    int status;

    thermal_power_estimator_init_one_element(&mock_dev_ctx, &mock_config);

    status = thermal_allocatable_power_calculate(
        &mock_dev_ctx, pid_output, &allocatable_power);

    TEST_ASSERT_EQUAL(FWK_SUCCESS, status);
    TEST_ASSERT_EQUAL(0, allocatable_power);
}

static void test_thermal_allocatable_power_calculate_over_max(void)
{
    struct mod_thermal_power_estimator_dev_ctx mock_dev_ctx = { 0 };
    struct mod_thermal_power_estimator_dev_config mock_config = {
        .tdp = 100,
    };
    int64_t pid_output = UINT32_MAX;
    uint32_t allocatable_power = 123;
    uint32_t expected_allocatable_power = 123;
    int status;

    thermal_power_estimator_init_one_element(&mock_dev_ctx, &mock_config);

    status = thermal_allocatable_power_calculate(
        &mock_dev_ctx, pid_output, &allocatable_power);

    TEST_ASSERT_EQUAL(FWK_E_DATA, status);
    TEST_ASSERT_EQUAL(expected_allocatable_power, allocatable_power);
}

void test_thermal_power_estimator_get_limit_success(void)
{
    fwk_id_t element_id = FWK_ID_ELEMENT_INIT(
        FWK_MODULE_IDX_THERMAL_POWER_ESTIMATOR,
        THERMAL_POWER_ESTIMATOR_FAKE_ELEM_IDX_0);
    struct mod_thermal_power_estimator_dev_ctx mock_dev_ctx = {
        .sensor_api = &mock_sensor_api,
        .pid_ctrl_api = &mock_pid_controller_api,
    };
    struct mod_thermal_power_estimator_dev_config mock_config = {
        .pid_controller_id =
            FWK_ID_ELEMENT_INIT(FWK_MODULE_IDX_PID_CONTROLLER, 0),
        .tdp = 0,
    };
    int64_t pid_output = 100;
    uint32_t power_limit = 0;
    int status;

    thermal_power_estimator_init_one_element(&mock_dev_ctx, &mock_config);
    fwk_id_get_element_idx_ExpectAndReturn(
        element_id, THERMAL_POWER_ESTIMATOR_FAKE_ELEM_IDX_0);

    mod_sensor_get_data_ExpectAndReturn(
        mock_config.sensor_id, &mock_dev_ctx.sensor_data, FWK_SUCCESS);

    /* Simulate PID controller update success */
    mod_pid_controller_update_ExpectAnyArgsAndReturn(FWK_SUCCESS);
    mod_pid_controller_update_ReturnThruPtr_output(&pid_output);

    status = thermal_power_estimator_get_limit(element_id, &power_limit);

    TEST_ASSERT_EQUAL(FWK_SUCCESS, status);
    TEST_ASSERT_EQUAL(pid_output, power_limit);
}

static void test_thermal_power_estimator_get_limit_sensor_fail(void)
{
    fwk_id_t element_id = FWK_ID_ELEMENT_INIT(
        FWK_MODULE_IDX_THERMAL_POWER_ESTIMATOR,
        THERMAL_POWER_ESTIMATOR_FAKE_ELEM_IDX_0);
    struct mod_thermal_power_estimator_dev_ctx mock_dev_ctx = {
        .sensor_api = &mock_sensor_api,
    };
    struct mod_thermal_power_estimator_dev_config mock_config = { 0 };
    uint32_t power_limit = 123, expected_power_limit = 123;
    int status;

    thermal_power_estimator_init_one_element(&mock_dev_ctx, &mock_config);
    fwk_id_get_element_idx_ExpectAndReturn(
        element_id, THERMAL_POWER_ESTIMATOR_FAKE_ELEM_IDX_0);

    mod_sensor_get_data_ExpectAndReturn(
        mock_config.sensor_id, &mock_dev_ctx.sensor_data, FWK_E_DEVICE);

    status = thermal_power_estimator_get_limit(element_id, &power_limit);

    TEST_ASSERT_EQUAL(FWK_E_DEVICE, status);
    TEST_ASSERT_EQUAL(expected_power_limit, power_limit);
}

static void test_thermal_power_estimator_get_limit_pid_fail(void)
{
    fwk_id_t element_id = FWK_ID_ELEMENT_INIT(
        FWK_MODULE_IDX_THERMAL_POWER_ESTIMATOR,
        THERMAL_POWER_ESTIMATOR_FAKE_ELEM_IDX_0);
    struct mod_thermal_power_estimator_dev_ctx mock_dev_ctx = {
        .sensor_api = &mock_sensor_api,
        .pid_ctrl_api = &mock_pid_controller_api,
    };
    struct mod_thermal_power_estimator_dev_config mock_config = {
        .pid_controller_id =
            FWK_ID_ELEMENT_INIT(FWK_MODULE_IDX_PID_CONTROLLER, 0),
        .tdp = 0,
    };
    int64_t pid_output = 100;
    uint32_t power_limit = 0;
    int status;

    thermal_power_estimator_init_one_element(&mock_dev_ctx, &mock_config);
    fwk_id_get_element_idx_ExpectAndReturn(
        element_id, THERMAL_POWER_ESTIMATOR_FAKE_ELEM_IDX_0);

    mod_sensor_get_data_ExpectAndReturn(
        mock_config.sensor_id, &mock_dev_ctx.sensor_data, FWK_SUCCESS);

    /* Simulate PID controller update success */
    mod_pid_controller_update_ExpectAnyArgsAndReturn(FWK_E_DEVICE);

    status = thermal_power_estimator_get_limit(element_id, &power_limit);

    TEST_ASSERT_EQUAL(FWK_E_DEVICE, status);
    TEST_ASSERT_NOT_EQUAL(pid_output, power_limit);
}

static void test_thermal_power_estimator_set_limit(void)
{
    fwk_id_t element_id = FWK_ID_ELEMENT_INIT(
        FWK_MODULE_IDX_THERMAL_POWER_ESTIMATOR,
        THERMAL_POWER_ESTIMATOR_FAKE_ELEM_IDX_0);
    uint32_t power_limit = 0;
    int status;

    status = thermal_power_estimator_set_limit(element_id, power_limit);

    TEST_ASSERT_EQUAL(FWK_E_SUPPORT, status);
}

int mod_thermal_power_estimator_test_main(void)
{
    UNITY_BEGIN();

    RUN_TEST(test_thermal_power_estimator_init_success);
    RUN_TEST(test_thermal_power_estimator_dev_init_success);
    RUN_TEST(test_thermal_power_estimator_bind_success);
    RUN_TEST(test_thermal_power_estimator_bind_sensor_bind_fail);
    RUN_TEST(test_thermal_power_estimator_process_bind_request_success);
    RUN_TEST(test_thermal_power_estimator_process_bind_request_invalid_api);
    RUN_TEST(test_read_temperature_success);
    RUN_TEST(test_read_temperature_fail_pending);
    RUN_TEST(test_read_temperature_fail_generic);
    RUN_TEST(test_thermal_allocatable_power_calculate_positive_output);
    RUN_TEST(test_thermal_allocatable_power_calculate_negative_output);
    RUN_TEST(test_thermal_allocatable_power_calculate_zero_output);
    RUN_TEST(test_thermal_allocatable_power_calculate_over_max);
    RUN_TEST(test_thermal_power_estimator_get_limit_success);
    RUN_TEST(test_thermal_power_estimator_get_limit_sensor_fail);
    RUN_TEST(test_thermal_power_estimator_get_limit_pid_fail);
    RUN_TEST(test_thermal_power_estimator_set_limit);

    return UNITY_END();
}

#if !defined(TEST_ON_TARGET)
int main(void)
{
    return mod_thermal_power_estimator_test_main();
}
#endif
