/*
 * Arm SCP/MCP Software
 * Copyright (c) 2025, Arm Limited and Contributors. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include "scp_unity.h"
#include "unity.h"

#include <Mockfwk_id.h>
#include <Mockfwk_module.h>
#include <Mockfwk_notification.h>

#include <mod_apcontext.h>
#include <mod_clock.h>

#include <fwk_element.h>

#include UNIT_TEST_SRC

void setUp(void)
{
    memset(&ctx, 0, sizeof(ctx));
}

void tearDown(void)
{
    /* Do Nothing */
}

void test_apcontext_init_success(void)
{
    const struct mod_apcontext_config config = {
        .base = 0x1000,
        .size = 100,
    };
    int status;

    status = apcontext_init(fwk_module_id_apcontext, 0, &config);
    TEST_ASSERT_EQUAL(FWK_SUCCESS, status);
}

void test_apcontext_init_with_elements_fail(void)
{
    int status = apcontext_init(fwk_module_id_apcontext, 1, NULL);
    TEST_ASSERT_EQUAL(FWK_E_PARAM, status);
}

void test_apcontext_init_with_null_config_fail(void)
{
    int status = apcontext_init(fwk_module_id_apcontext, 0, NULL);
    TEST_ASSERT_EQUAL(FWK_E_PARAM, status);
}

void test_apcontext_init_null_base_or_size_fail(void)
{
    struct mod_apcontext_config config = { 0 };
    int status;

    config.base = 0x0;
    config.size = 100;

    status = apcontext_init(fwk_module_id_apcontext, 0, &config);
    TEST_ASSERT_EQUAL(FWK_E_DATA, status);

    config.base = 0x1000;
    config.size = 0;

    status = apcontext_init(fwk_module_id_apcontext, 0, &config);
    TEST_ASSERT_EQUAL(FWK_E_DATA, status);
}

void test_apcontext_start_with_notifications_subscribe_success(void)
{
    const struct mod_apcontext_config config = {
        .base = 0x1000,
        .size = 100,
        .clock_id = FWK_ID_ELEMENT_INIT(FWK_MODULE_IDX_CLOCK, 0),
        .platform_notification.notification_id =
            FWK_ID_NOTIFICATION_INIT(FWK_MODULE_IDX_FAKE, 0),
        .platform_notification.source_id =
            FWK_ID_ELEMENT_INIT(FWK_MODULE_IDX_FAKE, 0),
    };
    int status;

    ctx.config = &config;

    fwk_id_is_equal_ExpectAndReturn(config.clock_id, FWK_ID_NONE, false);
    fwk_notification_subscribe_ExpectAnyArgsAndReturn(FWK_SUCCESS);

    fwk_id_type_is_valid_ExpectAndReturn(
        config.platform_notification.source_id, true);
    fwk_id_is_equal_ExpectAndReturn(
        config.platform_notification.source_id, FWK_ID_NONE, false);
    fwk_notification_subscribe_ExpectAnyArgsAndReturn(FWK_SUCCESS);

    status = apcontext_start(fwk_module_id_apcontext);
    TEST_ASSERT_EQUAL(FWK_SUCCESS, status);
    TEST_ASSERT_EQUAL(2, ctx.wait_on_notifications);
}

void test_apcontext_start_with_no_subscribers_success(void)
{
    uint8_t buff[100], expected;
    const struct mod_apcontext_config config = {
        .base = (uintptr_t)buff,
        .size = 100,
        .clock_id = FWK_ID_NONE,
        .platform_notification.source_id = FWK_ID_NONE,
    };
    int status;

    ctx.config = &config;

    fwk_id_is_equal_ExpectAndReturn(config.clock_id, FWK_ID_NONE, true);

    fwk_id_type_is_valid_ExpectAndReturn(
        config.platform_notification.source_id, true);
    fwk_id_is_equal_ExpectAndReturn(
        config.platform_notification.source_id, FWK_ID_NONE, true);
    expected = 0;

    status = apcontext_start(fwk_module_id_apcontext);
    TEST_ASSERT_EQUAL(FWK_SUCCESS, status);
    TEST_ASSERT_EQUAL(0, ctx.wait_on_notifications);
    TEST_ASSERT_EACH_EQUAL_UINT8(expected, buff, 100);
}

void test_apcontext_start_with_clock_notifications_fail(void)
{
    const struct mod_apcontext_config config = {
        .base = 0x1000,
        .size = 100,
        .clock_id = FWK_ID_ELEMENT_INIT(FWK_MODULE_IDX_CLOCK, 0),
        .platform_notification.source_id = FWK_ID_NONE,
    };
    int status;

    ctx.config = &config;

    fwk_id_is_equal_ExpectAndReturn(config.clock_id, FWK_ID_NONE, false);
    fwk_notification_subscribe_ExpectAnyArgsAndReturn(FWK_E_PARAM);

    status = apcontext_start(fwk_module_id_apcontext);
    TEST_ASSERT_EQUAL(FWK_E_PARAM, status);
    TEST_ASSERT_EQUAL(0, ctx.wait_on_notifications);
}

void test_apcontext_start_with_platform_notifications_fail(void)
{
    const struct mod_apcontext_config config = {
        .base = 0x1000,
        .size = 100,
        .clock_id = FWK_ID_NONE,
        .platform_notification.notification_id =
            FWK_ID_NOTIFICATION_INIT(FWK_MODULE_IDX_FAKE, 0),
        .platform_notification.source_id =
            FWK_ID_ELEMENT_INIT(FWK_MODULE_IDX_FAKE, 0),
    };
    int status;

    ctx.config = &config;

    fwk_id_is_equal_ExpectAndReturn(config.clock_id, FWK_ID_NONE, true);

    fwk_id_type_is_valid_ExpectAndReturn(
        config.platform_notification.source_id, true);
    fwk_id_is_equal_ExpectAndReturn(
        config.platform_notification.source_id, FWK_ID_NONE, false);
    fwk_notification_subscribe_ExpectAnyArgsAndReturn(FWK_E_PARAM);

    status = apcontext_start(fwk_module_id_apcontext);
    TEST_ASSERT_EQUAL(FWK_E_PARAM, status);
    TEST_ASSERT_EQUAL(0, ctx.wait_on_notifications);
}

void test_apcontext_zero(void)
{
    uint8_t buff[100], expected;
    const struct mod_apcontext_config config = {
        .base = (uintptr_t)buff,
        .size = 100,
    };

    ctx.config = &config;

    /* Fill buffer with some values and fill expected */
    memset(buff, 0xAA, 100);
    expected = 0;

    apcontext_zero();
    TEST_ASSERT_EACH_EQUAL_UINT8(expected, buff, 100);
}

void test_apcontext_process_clock_notification_success(void)
{
    uint8_t buff[100], expected;
    const struct mod_apcontext_config config = {
        .base = (uintptr_t)buff,
        .size = 100,
    };
    struct clock_notification_params params = {
        .new_state = MOD_CLOCK_STATE_RUNNING,
    };
    struct fwk_event clock_event = {
        .id = mod_clock_notification_id_state_changed,
        .source_id = FWK_ID_ELEMENT_INIT(FWK_MODULE_IDX_APCONTEXT, 0),
        .target_id = fwk_module_id_apcontext,
    };
    int status;

    ctx.config = &config;
    ctx.wait_on_notifications = 1;
    memcpy(&clock_event.params, &params, sizeof(params));

    fwk_id_is_type_ExpectAnyArgsAndReturn(true);
    fwk_id_is_equal_ExpectAndReturn(
        clock_event.id, mod_clock_notification_id_state_changed, true);
    fwk_id_is_equal_ExpectAnyArgsAndReturn(false);
    fwk_notification_unsubscribe_ExpectAnyArgsAndReturn(FWK_SUCCESS);
    expected = 0;

    status = apcontext_process_notification(&clock_event, NULL);
    TEST_ASSERT_EQUAL(FWK_SUCCESS, status);
    TEST_ASSERT_EQUAL(0, ctx.wait_on_notifications);
    TEST_ASSERT_EACH_EQUAL_UINT8(expected, buff, 100);
}

void test_apcontext_process_platform_notification_success(void)
{
    uint8_t buff[100], expected;
    const struct mod_apcontext_config config = {
        .base = (uintptr_t)buff,
        .size = 100,
        .platform_notification.notification_id =
            FWK_ID_NOTIFICATION_INIT(FWK_MODULE_IDX_FAKE, 0),
    };
    struct fwk_event platform_event = {
        .id = mod_clock_notification_id_state_changed,
        .source_id = fwk_module_id_fake,
        .target_id = fwk_module_id_apcontext,
    };
    int status;

    ctx.config = &config;
    ctx.wait_on_notifications = 1;

    fwk_id_is_type_ExpectAnyArgsAndReturn(true);
    fwk_id_is_equal_ExpectAnyArgsAndReturn(false);
    fwk_id_is_equal_ExpectAndReturn(
        platform_event.id, config.platform_notification.notification_id, true);
    fwk_notification_unsubscribe_ExpectAnyArgsAndReturn(FWK_SUCCESS);

    /* Fill buffer with some values and fill expected */
    memset(buff, 0xAA, 100);
    expected = 0;

    status = apcontext_process_notification(&platform_event, NULL);
    TEST_ASSERT_EQUAL(FWK_SUCCESS, status);
    TEST_ASSERT_EQUAL(0, ctx.wait_on_notifications);
    TEST_ASSERT_EACH_EQUAL_UINT8(expected, buff, 100);
}

void test_apcontext_process_clock_notification_fail(void)
{
    uint8_t buff[100], expected;
    const struct mod_apcontext_config config = {
        .base = (uintptr_t)buff,
        .size = 100,
    };
    struct clock_notification_params params = {
        .new_state = MOD_CLOCK_STATE_RUNNING,
    };
    struct fwk_event clock_event = {
        .id = mod_clock_notification_id_state_changed,
        .source_id = FWK_ID_ELEMENT_INIT(FWK_MODULE_IDX_APCONTEXT, 0),
        .target_id = fwk_module_id_apcontext,
    };
    int status;

    ctx.config = &config;
    ctx.wait_on_notifications = 1;
    memcpy(&clock_event.params, &params, sizeof(params));

    fwk_id_is_type_ExpectAnyArgsAndReturn(true);
    fwk_id_is_equal_ExpectAndReturn(
        clock_event.id, mod_clock_notification_id_state_changed, true);
    fwk_notification_unsubscribe_ExpectAnyArgsAndReturn(FWK_E_PARAM);

    /* Fill buffer with some values and fill expected */
    memset(buff, 0xAA, 100);
    expected = 0xAA;

    status = apcontext_process_notification(&clock_event, NULL);
    TEST_ASSERT_EQUAL(FWK_E_PARAM, status);
    TEST_ASSERT_EQUAL(1, ctx.wait_on_notifications);
    TEST_ASSERT_EACH_EQUAL_UINT8(expected, buff, 100);
}

void test_apcontext_process_platform_notification_fail(void)
{
    uint8_t buff[100], expected;
    const struct mod_apcontext_config config = {
        .base = (uintptr_t)buff,
        .size = 100,
        .platform_notification.notification_id =
            FWK_ID_NOTIFICATION_INIT(FWK_MODULE_IDX_FAKE, 0),
    };
    struct fwk_event platform_event = {
        .id = mod_clock_notification_id_state_changed,
        .source_id = fwk_module_id_fake,
        .target_id = fwk_module_id_apcontext,
    };
    int status;

    ctx.config = &config;
    ctx.wait_on_notifications = 1;

    fwk_id_is_type_ExpectAnyArgsAndReturn(true);
    fwk_id_is_equal_ExpectAnyArgsAndReturn(false);
    fwk_id_is_equal_ExpectAndReturn(
        platform_event.id, config.platform_notification.notification_id, true);
    fwk_notification_unsubscribe_ExpectAnyArgsAndReturn(FWK_E_PARAM);

    /* Fill buffer with some values and fill expected */
    memset(buff, 0xAA, 100);
    expected = 0xAA;

    status = apcontext_process_notification(&platform_event, NULL);
    TEST_ASSERT_EQUAL(FWK_E_PARAM, status);
    TEST_ASSERT_EQUAL(1, ctx.wait_on_notifications);
    TEST_ASSERT_EACH_EQUAL_UINT8(expected, buff, 100);
}

int apcontext_test_main(void)
{
    UNITY_BEGIN();

    RUN_TEST(test_apcontext_init_success);
    RUN_TEST(test_apcontext_init_with_elements_fail);
    RUN_TEST(test_apcontext_init_with_null_config_fail);
    RUN_TEST(test_apcontext_init_null_base_or_size_fail);

    RUN_TEST(test_apcontext_start_with_notifications_subscribe_success);
    RUN_TEST(test_apcontext_start_with_no_subscribers_success);
    RUN_TEST(test_apcontext_start_with_clock_notifications_fail);
    RUN_TEST(test_apcontext_start_with_platform_notifications_fail);

    RUN_TEST(test_apcontext_zero);

    RUN_TEST(test_apcontext_process_clock_notification_success);
    RUN_TEST(test_apcontext_process_platform_notification_success);
    RUN_TEST(test_apcontext_process_clock_notification_fail);
    RUN_TEST(test_apcontext_process_platform_notification_fail);

    return UNITY_END();
}

int main(void)
{
    return apcontext_test_main();
}
