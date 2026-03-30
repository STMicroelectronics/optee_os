/*
 * Arm SCP/MCP Software
 * Copyright (c) 2025, Arm Limited and Contributors. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include "Mockfwk_id.h"
#include "Mockfwk_module.h"
#include "Mockfwk_notification.h"
#include "Mocktest_fmu_extras.h"
#include "config_sbistc.h"
#include "unity.h"

#include <mod_fmu.h>
#include <mod_sbistc.h>

#include <fwk_id.h>
#include <fwk_log.h>
#include <fwk_notification.h>

#include <stdbool.h>
#include <string.h>

#include UNIT_TEST_SRC

/* Test configuration */
static bool handler_called = false;

static inline void test_handler(void)
{
    handler_called = true;
}

static const fwk_id_t sbistc_module_id = FWK_ID_MODULE(FWK_MODULE_IDX_SBISTC);

/* Test setup/teardown */
void setUp(void)
{
    sbistc_config = &test_config;
    fmu_api = &test_fmu_api;
}

void tearDown(void)
{
    fmu_api = NULL;
}

/* Test cases for sbistc_init() */
void test_sbistc_init_success(void)
{
    int status;
    status = sbistc_init(sbistc_module_id, 2, &test_config);
    TEST_ASSERT_EQUAL(FWK_SUCCESS, status);
}

void test_sbistc_init_null_data(void)
{
    TEST_ASSERT_EQUAL(FWK_E_PARAM, sbistc_init(sbistc_module_id, 0, NULL));
}

/* Test cases for sbistc_bind() */
void test_sbistc_bind_success_round0(void)
{
    fwk_id_t fmu_api_id =
        FWK_ID_API(FWK_MODULE_IDX_FMU, MOD_FMU_DEVICE_API_IDX);

    fwk_module_bind_ExpectAndReturn(
        FWK_ID_MODULE(FWK_MODULE_IDX_FMU),
        fmu_api_id,
        (const void **)&fmu_api,
        FWK_SUCCESS);

    int status = sbistc_bind(sbistc_module_id, 0);
    TEST_ASSERT_EQUAL(FWK_SUCCESS, status);
}

void test_sbistc_bind_success_round1(void)
{
    TEST_ASSERT_EQUAL(FWK_SUCCESS, sbistc_bind(sbistc_module_id, 1));
}

/* Test cases for sbistc_start() */
void test_sbistc_start_success(void)
{
    /* Mock fwk_module_get_data to ignore the actual data */
    fwk_module_get_data_IgnoreAndReturn(NULL);
    test_fmu_set_enabled_IgnoreAndReturn(FWK_SUCCESS);

    /* Mock notification subscription as expected by sbistc_start */
    fwk_notification_subscribe_ExpectAndReturn(
        mod_fmu_notification_id_fault,
        FWK_ID_MODULE(FWK_MODULE_IDX_FMU),
        sbistc_module_id,
        FWK_SUCCESS);
    fwk_id_is_type_ExpectAndReturn(
        FWK_ID_MODULE(FWK_MODULE_IDX_SBISTC), FWK_ID_TYPE_MODULE, true);

    TEST_ASSERT_EQUAL(FWK_SUCCESS, sbistc_start(sbistc_module_id));
}

void test_sbistc_start_null_api(void)
{
    fmu_api = NULL;
    TEST_ASSERT_EQUAL(FWK_E_STATE, sbistc_start(sbistc_module_id));
}

/* Test cases for sbistc_set_enabled() */
void test_sbistc_set_enabled_success(void)
{
    test_fmu_set_enabled_ExpectAnyArgsAndReturn(FWK_SUCCESS);
    test_fmu_set_enabled_ExpectAnyArgsAndReturn(FWK_SUCCESS);
    TEST_ASSERT_EQUAL(FWK_SUCCESS, sbistc_set_enabled(0, true));
    TEST_ASSERT_EQUAL(FWK_SUCCESS, sbistc_set_enabled(1, false));
}

void test_sbistc_set_enabled_invalid_id(void)
{
    TEST_ASSERT_EQUAL(FWK_E_PARAM, sbistc_set_enabled(2, true));
}

void test_sbistc_set_enabled_null_config(void)
{
    sbistc_config = NULL;
    TEST_ASSERT_EQUAL(FWK_E_PARAM, sbistc_set_enabled(0, true));
}

void test_sbistc_set_enabled_null_api(void)
{
    fmu_api = NULL;
    TEST_ASSERT_EQUAL(FWK_E_PARAM, sbistc_set_enabled(0, true));
}

/* Test cases for sbistc_set_handler() */
void test_sbistc_set_handler_success(void)
{
    TEST_ASSERT_EQUAL(FWK_SUCCESS, sbistc_set_handler(0, test_handler));
    TEST_ASSERT_EQUAL_PTR(test_handler, test_faults[0].handler);
}

void test_sbistc_set_handler_invalid_id(void)
{
    TEST_ASSERT_EQUAL(FWK_E_PARAM, sbistc_set_handler(2, test_handler));
}

void test_sbistc_set_handler_null_config(void)
{
    sbistc_config = NULL;
    TEST_ASSERT_EQUAL(FWK_E_PARAM, sbistc_set_handler(0, test_handler));
}

/* Test cases for sbistc_get_count() */
void test_sbistc_get_count_success(void)
{
    uint8_t count = 2;
    test_fmu_get_count_ExpectAndReturn(
        FWK_ID_ELEMENT(
            FWK_MODULE_IDX_FMU, test_faults[0].fmu_device_id), // Correct!
        test_faults[0].fmu_node_id,
        &count,
        FWK_SUCCESS);
    TEST_ASSERT_EQUAL(FWK_SUCCESS, sbistc_get_count(0, &count));
    TEST_ASSERT_EQUAL(2, count);
}

void test_sbistc_get_count_invalid_id(void)
{
    uint8_t count = 0;
    TEST_ASSERT_EQUAL(FWK_E_PARAM, sbistc_get_count(2, &count));
}

void test_sbistc_get_count_null_config(void)
{
    sbistc_config = NULL;
    uint8_t count = 0;
    TEST_ASSERT_EQUAL(FWK_E_PARAM, sbistc_get_count(0, &count));
}

void test_sbistc_get_count_null_api(void)
{
    fmu_api = NULL;
    uint8_t count = 0;
    TEST_ASSERT_EQUAL(FWK_E_PARAM, sbistc_get_count(0, &count));
}

void test_sbistc_get_count_null_count_ptr(void)
{
    TEST_ASSERT_EQUAL(FWK_E_PARAM, sbistc_get_count(0, NULL));
}

/* Test cases for sbistc_process_notification() */
void test_sbistc_process_notification_null_event(void)
{
    TEST_ASSERT_EQUAL(FWK_E_PARAM, sbistc_process_notification(NULL, NULL));
}

void test_sbistc_process_notification_null_config(void)
{
    sbistc_config = NULL;
    struct fwk_event event = { .id = sbistc_module_id };
    TEST_ASSERT_EQUAL(FWK_E_PARAM, sbistc_process_notification(&event, NULL));
}

void test_sbistc_process_notification_no_match(void)
{
    struct fwk_event event = { .id = sbistc_module_id,
                               .target_id =
                                   FWK_ID_MODULE(FWK_MODULE_IDX_SBISTC) };

    fwk_id_is_type_ExpectAndReturn(
        FWK_ID_MODULE(FWK_MODULE_IDX_SBISTC), FWK_ID_TYPE_MODULE, true);

    fwk_id_is_equal_ExpectAndReturn(
        event.id, mod_fmu_notification_id_fault, false);

    TEST_ASSERT_EQUAL(FWK_SUCCESS, sbistc_process_notification(&event, NULL));
}

void test_sbistc_process_notification_with_handler(void)
{
    struct mod_fmu_fault_notification_params params = {
        .fault.device_idx = 1,
        .fault.node_idx = 2,
    };
    struct fwk_event event = { 0 };
    event.id = mod_fmu_notification_id_fault;
    event.target_id = FWK_ID_MODULE(FWK_MODULE_IDX_SBISTC);
    memcpy(event.params, &params, sizeof(params));
    test_faults[0].handler = test_handler;
    handler_called = false;

    fwk_id_is_type_ExpectAndReturn(
        FWK_ID_MODULE(FWK_MODULE_IDX_SBISTC), FWK_ID_TYPE_MODULE, true);

    fwk_id_is_equal_ExpectAndReturn(
        event.id, mod_fmu_notification_id_fault, true);

    TEST_ASSERT_EQUAL(FWK_SUCCESS, sbistc_process_notification(&event, NULL));
    TEST_ASSERT_TRUE(handler_called);
}

void test_sbistc_process_notification_without_handler(void)
{
    struct mod_fmu_fault_notification_params params = {
        .fault.device_idx = 3,
        .fault.node_idx = 4,
    };
    struct fwk_event event = { 0 };
    event.id = mod_fmu_notification_id_fault;
    event.target_id = FWK_ID_MODULE(FWK_MODULE_IDX_SBISTC);
    memcpy(event.params, &params, sizeof(params));
    test_faults[1].handler = NULL;
    handler_called = false;

    fwk_id_is_type_ExpectAndReturn(
        FWK_ID_MODULE(FWK_MODULE_IDX_SBISTC), FWK_ID_TYPE_MODULE, true);

    fwk_id_is_equal_ExpectAndReturn(
        event.id, mod_fmu_notification_id_fault, true);

    TEST_ASSERT_EQUAL(FWK_SUCCESS, sbistc_process_notification(&event, NULL));
    TEST_ASSERT_FALSE(handler_called);
}

/* Test cases for sbistc_process_bind_request() */
void test_sbistc_process_bind_request_success(void)
{
    const void *api = NULL;
    fwk_id_t dummy_id = FWK_ID_MODULE(FWK_MODULE_IDX_SBISTC);

    int status =
        sbistc_process_bind_request(dummy_id, dummy_id, dummy_id, &api);

    TEST_ASSERT_EQUAL(FWK_SUCCESS, status);
    TEST_ASSERT_NOT_NULL(api);

    TEST_ASSERT_EQUAL_PTR(&sbistc_api, api);
}

/* Test cases for enable_fmu_parent_chain() */
void test_enable_fmu_parent_chain_simple(void)
{
    /* Setup a simple FMU tree: FMU1 -> FMU0 (root) */
    struct mod_fmu_dev_config fmu1_cfg = {
        /*! FMU0 is parent */
        .parent = 0,

        /*! Node 1 on FMU0 */
        .parent_ncr_index = 1,

        /*! Node 0 on FMU0 */
        .parent_cr_index = 0,
    };
    struct mod_fmu_dev_config fmu0_cfg = {
        .parent = MOD_FMU_PARENT_NONE,
    };

    /* First call: get FMU1 config */
    fwk_module_get_data_ExpectAndReturn(
        FWK_ID_ELEMENT(FWK_MODULE_IDX_FMU, 1), &fmu1_cfg);
    /* Second call: get FMU0 config */
    fwk_module_get_data_ExpectAndReturn(
        FWK_ID_ELEMENT(FWK_MODULE_IDX_FMU, 0), &fmu0_cfg);

    test_fmu_set_enabled_ExpectAnyArgsAndReturn(FWK_SUCCESS);

    int status = enable_fmu_parent_chain(1, false);
    TEST_ASSERT_EQUAL(FWK_SUCCESS, status);
}

/* Main runner */
int main(void)
{
    UNITY_BEGIN();

    RUN_TEST(test_sbistc_init_success);
    RUN_TEST(test_sbistc_init_null_data);

    RUN_TEST(test_sbistc_bind_success_round0);
    RUN_TEST(test_sbistc_bind_success_round1);

    RUN_TEST(test_sbistc_start_success);
    RUN_TEST(test_sbistc_start_null_api);

    RUN_TEST(test_sbistc_set_enabled_success);
    RUN_TEST(test_sbistc_set_enabled_invalid_id);
    RUN_TEST(test_sbistc_set_enabled_null_config);
    RUN_TEST(test_sbistc_set_enabled_null_api);

    RUN_TEST(test_sbistc_set_handler_success);
    RUN_TEST(test_sbistc_set_handler_invalid_id);
    RUN_TEST(test_sbistc_set_handler_null_config);

    RUN_TEST(test_sbistc_get_count_success);
    RUN_TEST(test_sbistc_get_count_invalid_id);
    RUN_TEST(test_sbistc_get_count_null_config);
    RUN_TEST(test_sbistc_get_count_null_api);
    RUN_TEST(test_sbistc_get_count_null_count_ptr);

    RUN_TEST(test_sbistc_process_notification_null_event);
    RUN_TEST(test_sbistc_process_notification_null_config);
    RUN_TEST(test_sbistc_process_notification_no_match);
    RUN_TEST(test_sbistc_process_notification_with_handler);
    RUN_TEST(test_sbistc_process_notification_without_handler);

    RUN_TEST(test_sbistc_process_bind_request_success);

    RUN_TEST(test_enable_fmu_parent_chain_simple);

    return UNITY_END();
}
