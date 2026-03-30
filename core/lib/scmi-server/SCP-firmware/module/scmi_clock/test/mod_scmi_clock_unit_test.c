/*
 * Arm SCP/MCP Software
 * Copyright (c) 2022-2025, Arm Limited and Contributors. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include "scp_unity.h"
#include "unity.h"
#ifdef TEST_ON_TARGET
#    include <fwk_id.h>
#    include <fwk_module.h>
#else
#    include <Mockfwk_id.h>
#    include <Mockfwk_mm.h>
#    include <Mockfwk_module.h>
#    include <Mockfwk_notification.h>
#    include <internal/Mockfwk_core_internal.h>
#    include <internal/Mockfwk_id_internal.h>
#endif
#include <Mockmod_scmi_clock_extra.h>
#include <mod_clock.h>

#include <mod_scmi.h>
#include <scmi_clock.h>

#include <fwk_element.h>
#include <fwk_macros.h>

#if defined(BUILD_HAS_MOD_RESOURCE_PERMS)
    #include <mod_resource_perms.h>
#endif

#include UNIT_TEST_SRC
#include <config_scmi_clock.h>

static struct mod_scmi_clock_ctx scmi_clock_ctx;

struct mod_scmi_from_protocol_api from_protocol_api = {
    .get_agent_count = mod_scmi_from_protocol_api_get_agent_count,
    .get_agent_id = mod_scmi_from_protocol_api_get_agent_id,
    .get_agent_type = mod_scmi_from_protocol_api_get_agent_type,
    .get_max_payload_size = mod_scmi_from_protocol_api_get_max_payload_size,
    .write_payload = mod_scmi_from_protocol_api_write_payload,
    .respond = mod_scmi_from_protocol_api_respond,
    .scmi_message_validation = mod_scmi_from_protocol_api_scmi_frame_validation,
    .notify = mod_scmi_from_protocol_api_notify,
};

struct mod_clock_api clock_api = {
    .get_state = mod_clock_api_get_state,
};

static char *mock_clock_name = "TestClockDevice001";

#if defined(BUILD_HAS_MOD_RESOURCE_PERMS)
struct mod_res_permissions_api perm_api = {
    .agent_has_protocol_permission = mod_res_permissions_api_agent_has_protocol_permission,
    .agent_has_message_permission = mod_res_permissions_api_agent_has_message_permission,
    .agent_has_resource_permission = mod_res_permissions_api_agent_has_resource_permission,
    .agent_set_device_permission = mod_res_permissions_api_agent_set_device_permission,
    .agent_set_device_protocol_permission = mod_res_permissions_api_agent_set_device_protocol_permission,
    .agent_reset_config = mod_res_permissions_api_agent_reset_config,
};
#endif

void assert_ospm0_clock_state_meets_expectations(void)
{
    TEST_ASSERT_EQUAL_UINT8_ARRAY(
        ospm0_state_table_expected,
        scmi_clock_ctx.agent_table[FAKE_SCMI_AGENT_IDX_OSPM0]->state_table,
        SCMI_CLOCK_OSPM0_COUNT);
}

void assert_ospm1_clock_state_meets_expectations(void)
{
    TEST_ASSERT_EQUAL_UINT8_ARRAY(
        ospm1_state_table_expected,
        scmi_clock_ctx.agent_table[FAKE_SCMI_AGENT_IDX_OSPM1]->state_table,
        SCMI_CLOCK_OSPM1_COUNT);
}

void assert_ref_count_meets_expectations(void)
{
    TEST_ASSERT_EQUAL_UINT8_ARRAY(
        dev_clock_ref_count_table_expected,
        dev_clock_ref_count_table,
        CLOCK_DEV_IDX_COUNT);
}

void setup_agent_state_table(
    unsigned int agent_id,
    unsigned int scmi_clock_idx,
    enum mod_clock_state state)
{
    scmi_clock_ctx.agent_table[agent_id]->state_table[scmi_clock_idx] = (unsigned int)state;
}

void setup_ref_count_table(
    unsigned int clock_idx,
    unsigned int ref_count)
{
    dev_clock_ref_count_table[clock_idx] = ref_count;
}

void setup_expected_ospm0_state_table(
    unsigned int scmi_clock_idx,
    enum mod_clock_state state)
{
    ospm0_state_table_expected[scmi_clock_idx] = (unsigned int)state;
}

void setup_expected_ospm1_state_table(
    unsigned int scmi_clock_idx,
    enum mod_clock_state state)
{
    ospm1_state_table_expected[scmi_clock_idx] = (unsigned int)state;
}

void setup_expected_ref_count_table(
    unsigned int clock_idx,
    unsigned int ref_count)
{
    dev_clock_ref_count_table_expected[clock_idx] = ref_count;
}


void setUp(void)
{
    struct mod_scmi_clock_config *data = (struct mod_scmi_clock_config *)config_scmi_clock.data;
    scmi_clock_ctx.max_pending_transactions = data->max_pending_transactions;

    scmi_clock_ctx.phy_device_count = CLOCK_DEV_IDX_COUNT;
    scmi_clock_ctx.agent_count = FAKE_SCMI_AGENT_IDX_COUNT;
    scmi_clock_ctx.agent_table = agent_table;

    /* Allocate a table of clock operations */
    scmi_clock_ctx.clock_ops = clock_ops_table;
    memset(clock_ops_table, 0,
           CLOCK_DEV_IDX_COUNT * sizeof(struct clock_operations));

    /* Initialize table */
    for (unsigned int i = 0; i < (unsigned int)scmi_clock_ctx.phy_device_count; i++) {
        scmi_clock_ctx.clock_ops[i].service_id = FWK_ID_NONE;
    }

    scmi_clock_ctx.scmi_api = &from_protocol_api;
    scmi_clock_ctx.clock_api = &clock_api;
    #if defined(BUILD_HAS_MOD_RESOURCE_PERMS)
        scmi_clock_ctx.res_perms_api = &perm_api;
    #endif

    scmi_clock_ctx.dev_clock_ref_count_table = dev_clock_ref_count_table;

    #if defined(BUILD_HAS_SCMI_NOTIFICATIONS)
    scmi_clock_ctx.notification_table = operation_table_default;
    #endif

    memcpy(
        dev_clock_ref_count_table,
        dev_clock_ref_count_table_default,
        FWK_ARRAY_SIZE(dev_clock_ref_count_table));
    memcpy(
        dev_clock_ref_count_table_expected,
        dev_clock_ref_count_table_default,
        FWK_ARRAY_SIZE(dev_clock_ref_count_table));

    memcpy(ospm0_state_table,
        ospm0_state_table_default,
        FWK_ARRAY_SIZE(ospm0_state_table));
    memcpy(ospm1_state_table,
        ospm1_state_table_default,
        FWK_ARRAY_SIZE(ospm1_state_table));

    memcpy(
        ospm0_state_table_expected,
        ospm0_state_table_default,
        FWK_ARRAY_SIZE(ospm0_state_table_default));
    memcpy(
        ospm1_state_table_expected,
        ospm1_state_table_default,
        FWK_ARRAY_SIZE(ospm1_state_table_default));

}

void tearDown(void)
{
    Mockmod_scmi_clock_extra_Verify();
    Mockmod_scmi_clock_extra_Destroy();
    Mockfwk_core_internal_Verify();
    Mockfwk_core_internal_Destroy();
    Mockfwk_id_Verify();
    Mockfwk_id_Destroy();
}

int fwk_put_event_callback(struct fwk_event *event, int numCalls)
{
    struct scmi_clock_event_request_params *params;
    params = (struct scmi_clock_event_request_params *)event->params;

    TEST_ASSERT_EQUAL(0x00000001, params->request_data.set_rate_data.rate[0]);
    TEST_ASSERT_EQUAL(0x00000001, params->request_data.set_rate_data.rate[1]);
    TEST_ASSERT_EQUAL(MOD_CLOCK_ROUND_MODE_NEAREST,
                      params->request_data.set_rate_data.round_mode);

    return FWK_SUCCESS;
}

unsigned int get_element_idx_callback(
    fwk_id_t clock_id,
    int NumCalls)
{
    return clock_id.element.element_idx;
}

void test_function_set_rate(void)
{
    int status;

    fwk_id_t service_id =
        FWK_ID_ELEMENT_INIT(FAKE_MODULE_IDX, FAKE_SCMI_AGENT_IDX_OSPM0);

    unsigned int agent_id = FAKE_SCMI_AGENT_IDX_OSPM0;

    struct scmi_clock_rate_set_a2p payload = {
        .flags = SCMI_CLOCK_RATE_SET_ROUND_AUTO_MASK,
        .clock_id = CLOCK_DEV_IDX_FAKE0,
        .rate[0] = 0x00000001,
        .rate[1] = 0x00000001,
    };

    mod_scmi_from_protocol_api_get_agent_id_ExpectAnyArgsAndReturn(FWK_SUCCESS);
    mod_scmi_from_protocol_api_get_agent_id_ReturnThruPtr_agent_id(&agent_id);

    #if defined(BUILD_HAS_MOD_RESOURCE_PERMS)
        mod_res_permissions_api_agent_has_resource_permission_ExpectAnyArgsAndReturn(MOD_RES_PERMS_ACCESS_ALLOWED);
    #endif

    fwk_module_is_valid_element_id_ExpectAnyArgsAndReturn(true);
    mod_scmi_from_protocol_api_scmi_frame_validation_ExpectAnyArgsAndReturn(
            FWK_SUCCESS);

    fwk_id_get_element_idx_ExpectAnyArgsAndReturn(CLOCK_DEV_IDX_FAKE0);
    fwk_id_is_equal_ExpectAnyArgsAndReturn(true);

    __fwk_put_event_Stub(fwk_put_event_callback);

    status = scmi_clock_message_handler(
                                    (fwk_id_t)MOD_SCMI_PROTOCOL_ID_CLOCK,
                                    service_id,
                                    (const uint32_t *)&payload,
                                    payload_size_table[MOD_SCMI_CLOCK_RATE_SET],
                                    MOD_SCMI_CLOCK_RATE_SET);

    TEST_ASSERT_EQUAL(FWK_SUCCESS, status);
}

void test_set_rate_with_invalid_message_id_expect_SCMI_NOT_FOUND(void) {
    int status;
    int32_t return_value = SCMI_NOT_FOUND;
    fwk_id_t service_id =
        FWK_ID_ELEMENT_INIT(FAKE_MODULE_IDX, FAKE_SCMI_AGENT_IDX_OSPM0);

    struct scmi_clock_rate_set_a2p payload = {
        .flags = SCMI_CLOCK_RATE_SET_ROUND_AUTO_MASK,
        .clock_id = CLOCK_DEV_IDX_FAKE0,
        .rate[0] = 0x00000001,
        .rate[1] = 0x00000001,
    };

    unsigned int invalid_message_ids[] = {
        /* Sanity value of max unsigned int */
        -1,
        /* Just after the valid range */
        MOD_SCMI_CLOCK_COMMAND_COUNT,
        /* A median value of the invalid range */
        (-1 - MOD_SCMI_CLOCK_COMMAND_COUNT) / 2};

    for (unsigned int i = 0; i < FWK_ARRAY_SIZE(invalid_message_ids); ++i) {
      mod_scmi_from_protocol_api_respond_ExpectAndReturn(service_id,
                                                         &return_value,
                                                         sizeof(return_value),
                                                         FWK_SUCCESS);

    mod_scmi_from_protocol_api_scmi_frame_validation_ExpectAnyArgsAndReturn(
        SCMI_NOT_FOUND);

      status = scmi_clock_message_handler(
            (fwk_id_t)MOD_SCMI_PROTOCOL_ID_CLOCK,
            service_id,
            (const uint32_t *)&payload,
            payload_size_table[MOD_SCMI_CLOCK_RATE_SET],
            invalid_message_ids[i]);

      TEST_ASSERT_EQUAL(FWK_SUCCESS, status);
    }
}

void test_set_rate_with_invalid_payload_size_expect_SCMI_PROTOCOL_ERROR(void) {
    int status;
    int32_t return_value = SCMI_PROTOCOL_ERROR;

    fwk_id_t service_id =
      FWK_ID_ELEMENT_INIT(FAKE_MODULE_IDX, FAKE_SCMI_AGENT_IDX_OSPM0);

    struct scmi_clock_rate_set_a2p payload = {
        .flags = SCMI_CLOCK_RATE_SET_ROUND_AUTO_MASK,
        .clock_id = CLOCK_DEV_IDX_FAKE0,
        .rate[0] = 0x00000001,
        .rate[1] = 0x00000001,
    };

    unsigned int invalid_payload_sizes[] = {0, -1, /* sanity values */
                                            sizeof(payload) + 1,
                                            sizeof(payload) - 1};

    for (unsigned int i = 0; i < FWK_ARRAY_SIZE(invalid_payload_sizes); ++i) {
      mod_scmi_from_protocol_api_respond_ExpectAndReturn(service_id,
                                                         &return_value,
                                                         sizeof(return_value),
                                                         FWK_SUCCESS);

    mod_scmi_from_protocol_api_scmi_frame_validation_ExpectAnyArgsAndReturn(
        SCMI_PROTOCOL_ERROR);

      status = scmi_clock_message_handler((fwk_id_t)MOD_SCMI_PROTOCOL_ID_CLOCK,
                                          service_id,
                                          (const uint32_t *)&payload,
                                          invalid_payload_sizes[i],
                                          MOD_SCMI_CLOCK_RATE_SET);

      TEST_ASSERT_EQUAL(FWK_SUCCESS, status);
    }
}

void test_clock_ref_count_allocate(void)
{
    uint8_t *dev_clock_ref_count_table_return = (uint8_t *)0xAAAAAAAA;

    scmi_clock_ctx.dev_clock_ref_count_table = NULL;

    fwk_mm_calloc_ExpectAndReturn(
            (unsigned int)scmi_clock_ctx.phy_device_count,
            sizeof(*scmi_clock_ctx.dev_clock_ref_count_table),
            dev_clock_ref_count_table_return);

    clock_ref_count_allocate();

    TEST_ASSERT_EQUAL_PTR(
        dev_clock_ref_count_table_return,
        scmi_clock_ctx.dev_clock_ref_count_table);
}

void test_mod_scmi_clock_request_state_check_no_change_running(void)
{
    enum mod_scmi_clock_policy_status status;
    fwk_id_get_element_idx_ExpectAnyArgsAndReturn(CLOCK_DEV_IDX_FAKE0);

    /* Set `SCMI_CLOCK_OSPM0_IDX0` as RUNNING and ref_count == 1 */
    setup_agent_state_table(
        FAKE_SCMI_AGENT_IDX_OSPM0,
        SCMI_CLOCK_OSPM0_IDX0,
        MOD_CLOCK_STATE_RUNNING);
    setup_ref_count_table(CLOCK_DEV_IDX_FAKE0, 1);

    status = scmi_clock_ref_count_table_check(
        FAKE_SCMI_AGENT_IDX_OSPM0,
        SCMI_CLOCK_OSPM0_IDX0,
        MOD_CLOCK_STATE_RUNNING);
    TEST_ASSERT_EQUAL(MOD_SCMI_CLOCK_SKIP_MESSAGE_HANDLER, status);
}

void test_mod_scmi_clock_request_state_check_no_change_stopped(void)
{
    enum mod_scmi_clock_policy_status status;
    fwk_id_get_element_idx_ExpectAnyArgsAndReturn(CLOCK_DEV_IDX_FAKE0);

    /* Set `SCMI_CLOCK_OSPM0_IDX0` as STOPPED and ref_count == 0  */
    setup_agent_state_table(
        FAKE_SCMI_AGENT_IDX_OSPM0,
        SCMI_CLOCK_OSPM0_IDX0,
        MOD_CLOCK_STATE_STOPPED);
    setup_ref_count_table(CLOCK_DEV_IDX_FAKE0, 0);

    status = scmi_clock_ref_count_table_check(
        FAKE_SCMI_AGENT_IDX_OSPM0,
        SCMI_CLOCK_OSPM0_IDX0,
        MOD_CLOCK_STATE_STOPPED);
    TEST_ASSERT_EQUAL(MOD_SCMI_CLOCK_SKIP_MESSAGE_HANDLER, status);
}

void test_mod_scmi_clock_request_state_check_ref_count_0_running(void)
{
    enum mod_scmi_clock_policy_status status;
    fwk_id_get_element_idx_ExpectAnyArgsAndReturn(CLOCK_DEV_IDX_FAKE0);

    /* Set `SCMI_CLOCK_OSPM0_IDX0` as STOPPED and ref_count == 0 */
    setup_agent_state_table(
        FAKE_SCMI_AGENT_IDX_OSPM0,
        SCMI_CLOCK_OSPM0_IDX0,
        MOD_CLOCK_STATE_STOPPED);
    setup_ref_count_table(CLOCK_DEV_IDX_FAKE0, 0);

    status = scmi_clock_ref_count_table_check(
        FAKE_SCMI_AGENT_IDX_OSPM0,
        SCMI_CLOCK_OSPM0_IDX0,
        MOD_CLOCK_STATE_RUNNING);
    TEST_ASSERT_EQUAL(MOD_SCMI_CLOCK_EXECUTE_MESSAGE_HANDLER, status);
}

void test_mod_scmi_clock_request_state_check_ref_count_1_stopped(void)
{

    enum mod_scmi_clock_policy_status status;
    fwk_id_get_element_idx_ExpectAnyArgsAndReturn(CLOCK_DEV_IDX_FAKE0);

    /* Set `SCMI_CLOCK_OSPM0_IDX0` as RUNNING and ref_count == 1 */
    setup_agent_state_table(
        FAKE_SCMI_AGENT_IDX_OSPM0,
        SCMI_CLOCK_OSPM0_IDX0,
        MOD_CLOCK_STATE_RUNNING);
    setup_ref_count_table(CLOCK_DEV_IDX_FAKE0, 1);

    status = scmi_clock_ref_count_table_check(
        FAKE_SCMI_AGENT_IDX_OSPM0,
        SCMI_CLOCK_OSPM0_IDX0,
        MOD_CLOCK_STATE_STOPPED);
    TEST_ASSERT_EQUAL(MOD_SCMI_CLOCK_EXECUTE_MESSAGE_HANDLER, status);
}

void test_mod_scmi_clock_request_state_check_ref_count_0_stopped(void)
{
    enum mod_scmi_clock_policy_status status;
    fwk_id_get_element_idx_ExpectAnyArgsAndReturn(CLOCK_DEV_IDX_FAKE0);

    /*
     * Create an invalid situation where it is set `SCMI_CLOCK_OSPM0_IDX0`
     * as RUNNING and ref_count == 0
     */
    setup_agent_state_table(
        FAKE_SCMI_AGENT_IDX_OSPM0,
        SCMI_CLOCK_OSPM0_IDX0,
        MOD_CLOCK_STATE_RUNNING);
    setup_ref_count_table(CLOCK_DEV_IDX_FAKE0, 0);

    status = scmi_clock_ref_count_table_check(
        FAKE_SCMI_AGENT_IDX_OSPM0,
        SCMI_CLOCK_OSPM0_IDX0,
        MOD_CLOCK_STATE_STOPPED);
    TEST_ASSERT_EQUAL(MOD_SCMI_CLOCK_SKIP_MESSAGE_HANDLER, status);
}

void test_mod_scmi_clock_request_state_check_ref_count_1_running(void)
{
    enum mod_scmi_clock_policy_status status;
    fwk_id_get_element_idx_ExpectAnyArgsAndReturn(CLOCK_DEV_IDX_FAKE3);

    /*
     * Set `SCMI_CLOCK_OSPM0_IDX3` as RUNNING,
     * set `SCMI_CLOCK_OSPM1_IDX0` as STOPPED and
     * ref_count == 1
     */
    setup_agent_state_table(
        FAKE_SCMI_AGENT_IDX_OSPM0,
        SCMI_CLOCK_OSPM0_IDX3,
        MOD_CLOCK_STATE_RUNNING);
    setup_agent_state_table(
        FAKE_SCMI_AGENT_IDX_OSPM1,
        SCMI_CLOCK_OSPM1_IDX0,
        MOD_CLOCK_STATE_STOPPED);
    setup_ref_count_table(CLOCK_DEV_IDX_FAKE3, 1);

    status = scmi_clock_ref_count_table_check(
        FAKE_SCMI_AGENT_IDX_OSPM1,
        SCMI_CLOCK_OSPM1_IDX0,
        MOD_CLOCK_STATE_RUNNING);
    TEST_ASSERT_EQUAL(MOD_SCMI_CLOCK_SKIP_MESSAGE_HANDLER, status);
}

void test_mod_scmi_clock_request_state_check_ref_count_2_stopped(void)
{
    enum mod_scmi_clock_policy_status status;
    fwk_id_get_element_idx_ExpectAnyArgsAndReturn(CLOCK_DEV_IDX_FAKE3);

    /*
     * Set `SCMI_CLOCK_OSPM0_IDX3` as RUNNING,
     * set `SCMI_CLOCK_OSPM1_IDX0` as RUNNING and
     * ref_count == 2
     */
    setup_agent_state_table(
        FAKE_SCMI_AGENT_IDX_OSPM0,
        SCMI_CLOCK_OSPM0_IDX3,
        MOD_CLOCK_STATE_RUNNING);
    setup_agent_state_table(
        FAKE_SCMI_AGENT_IDX_OSPM1,
        SCMI_CLOCK_OSPM1_IDX0,
        MOD_CLOCK_STATE_RUNNING);
    setup_ref_count_table(CLOCK_DEV_IDX_FAKE3, 2);

    status = scmi_clock_ref_count_table_check(
        FAKE_SCMI_AGENT_IDX_OSPM1,
        SCMI_CLOCK_OSPM1_IDX0,
        MOD_CLOCK_STATE_STOPPED);
    TEST_ASSERT_EQUAL(MOD_SCMI_CLOCK_SKIP_MESSAGE_HANDLER, status);
}

void test_mod_scmi_clock_state_update_no_change_running(void)
{
    int status;
    uint32_t agent_id = FAKE_SCMI_AGENT_IDX_OSPM0;
    fwk_id_t service_id = FWK_ID_ELEMENT_INIT(FAKE_MODULE_IDX, agent_id);
    enum mod_clock_state requested_state = MOD_CLOCK_STATE_RUNNING;
    enum mod_scmi_clock_policy_status policy_status = MOD_SCMI_CLOCK_SKIP_MESSAGE_HANDLER;

    mod_scmi_from_protocol_api_get_agent_id_ExpectAnyArgsAndReturn(FWK_SUCCESS);
    mod_scmi_from_protocol_api_get_agent_id_ReturnThruPtr_agent_id(&agent_id);
    /*
     * Set `SCMI_CLOCK_OSPM0_IDX0` as RUNNING,
     * set expected `SCMI_CLOCK_OSPM0_IDX0` as RUNNING and
     * ref_count == 1 and expected ref_count == 1
     */
    setup_agent_state_table(
        FAKE_SCMI_AGENT_IDX_OSPM0,
        SCMI_CLOCK_OSPM0_IDX0,
        MOD_CLOCK_STATE_RUNNING);
    setup_ref_count_table(CLOCK_DEV_IDX_FAKE0, 1);

    setup_expected_ospm0_state_table(
        SCMI_CLOCK_OSPM0_IDX0,
        MOD_CLOCK_STATE_RUNNING);
    setup_expected_ref_count_table(CLOCK_DEV_IDX_FAKE0, 1);

    status = mod_scmi_clock_config_set_policy(
        &policy_status,
        &requested_state,
        MOD_SCMI_CLOCK_PRE_MESSAGE_HANDLER,
        service_id,
        SCMI_CLOCK_OSPM0_IDX0);

    TEST_ASSERT_EQUAL(FWK_SUCCESS, status);
    assert_ospm0_clock_state_meets_expectations();
    assert_ref_count_meets_expectations();
}

void test_mod_scmi_clock_state_update_no_change_stopped(void)
{
    int status;
    uint32_t agent_id = FAKE_SCMI_AGENT_IDX_OSPM0;
    fwk_id_t service_id = FWK_ID_ELEMENT_INIT(FAKE_MODULE_IDX, agent_id);
    enum mod_clock_state requested_state = MOD_CLOCK_STATE_STOPPED;
    enum mod_scmi_clock_policy_status policy_status = MOD_SCMI_CLOCK_SKIP_MESSAGE_HANDLER;

    mod_scmi_from_protocol_api_get_agent_id_ExpectAnyArgsAndReturn(FWK_SUCCESS);
    mod_scmi_from_protocol_api_get_agent_id_ReturnThruPtr_agent_id(&agent_id);

    /*
     * Set `SCMI_CLOCK_OSPM0_IDX0` as STOPPED,
     * set expected `SCMI_CLOCK_OSPM0_IDX0` as STOPPED,
     * ref_count == 0 and expected ref_count == 0
     */
    setup_agent_state_table(
        FAKE_SCMI_AGENT_IDX_OSPM0,
        SCMI_CLOCK_OSPM0_IDX0,
        MOD_CLOCK_STATE_STOPPED);
    setup_ref_count_table(CLOCK_DEV_IDX_FAKE0, 0);

    setup_expected_ospm0_state_table(
        SCMI_CLOCK_OSPM0_IDX0,
        MOD_CLOCK_STATE_STOPPED);
    setup_expected_ref_count_table(CLOCK_DEV_IDX_FAKE0, 0);

    status = mod_scmi_clock_config_set_policy(
        &policy_status,
        &requested_state,
        MOD_SCMI_CLOCK_PRE_MESSAGE_HANDLER,
        service_id,
        SCMI_CLOCK_OSPM0_IDX0);
    TEST_ASSERT_EQUAL(FWK_SUCCESS, status);

    assert_ospm0_clock_state_meets_expectations();
    assert_ref_count_meets_expectations();
}

void test_mod_scmi_clock_state_update_ref_count_0_running(void)
{
    int status;

    /*
     * Set SCMI clock 0 from `FAKE_SCMI_AGENT_IDX_OSPM0` as STOPPED,
     * set expected SCMI clock 0 from `FAKE_SCMI_AGENT_IDX_OSPM0` as RUNNING,
     * ref_count == 0 and expected ref_count == 1
     */
    setup_agent_state_table(
        FAKE_SCMI_AGENT_IDX_OSPM0,
        SCMI_CLOCK_OSPM0_IDX0,
        MOD_CLOCK_STATE_STOPPED);
    setup_ref_count_table(CLOCK_DEV_IDX_FAKE0, 0);

    setup_expected_ospm0_state_table(
        SCMI_CLOCK_OSPM0_IDX0,
        MOD_CLOCK_STATE_RUNNING);
    setup_expected_ref_count_table(CLOCK_DEV_IDX_FAKE0, 1);

    fwk_id_get_element_idx_ExpectAnyArgsAndReturn(CLOCK_DEV_IDX_FAKE0);

    status = mod_scmi_clock_state_update(
        FAKE_SCMI_AGENT_IDX_OSPM0,
        SCMI_CLOCK_OSPM0_IDX0,
        MOD_CLOCK_STATE_RUNNING);
    TEST_ASSERT_EQUAL(FWK_SUCCESS, status);

    assert_ospm0_clock_state_meets_expectations();
    assert_ref_count_meets_expectations();
}

void test_mod_scmi_clock_state_update_ref_count_1_running(void)
{
    int status;

    /*
     * Set `SCMI_CLOCK_OSPM0_IDX3` as RUNNING,
     * set `SCMI_CLOCK_OSPM1_IDX0` as STOPPED,
     * set expected `SCMI_CLOCK_OSPM0_IDX3` as RUNNING,
     * set expected `SCMI_CLOCK_OSPM1_IDX0` as RUNNING,
     * ref_count == 2 and expected ref_count == 1
     */
    setup_agent_state_table(
        FAKE_SCMI_AGENT_IDX_OSPM0,
        SCMI_CLOCK_OSPM0_IDX3,
        MOD_CLOCK_STATE_RUNNING);
    setup_agent_state_table(
        FAKE_SCMI_AGENT_IDX_OSPM1,
        SCMI_CLOCK_OSPM1_IDX0,
        MOD_CLOCK_STATE_STOPPED);
    setup_ref_count_table(CLOCK_DEV_IDX_FAKE3, 1);

    setup_expected_ospm0_state_table(
        SCMI_CLOCK_OSPM0_IDX3,
        MOD_CLOCK_STATE_RUNNING);
    setup_expected_ospm1_state_table(
        SCMI_CLOCK_OSPM1_IDX0,
        MOD_CLOCK_STATE_RUNNING);
    setup_expected_ref_count_table(CLOCK_DEV_IDX_FAKE3, 2);

    fwk_id_get_element_idx_ExpectAnyArgsAndReturn(CLOCK_DEV_IDX_FAKE3);

    status = mod_scmi_clock_state_update(
        FAKE_SCMI_AGENT_IDX_OSPM1,
        SCMI_CLOCK_OSPM1_IDX0,
        MOD_CLOCK_STATE_RUNNING);
    TEST_ASSERT_EQUAL(FWK_SUCCESS, status);

    assert_ospm0_clock_state_meets_expectations();
    assert_ospm1_clock_state_meets_expectations();
    assert_ref_count_meets_expectations();
}

void test_mod_scmi_clock_state_update_ref_count_2_stopped(void)
{
    int status;

    /*
     * Set `SCMI_CLOCK_OSPM0_IDX3` as RUNNING,
     * set `SCMI_CLOCK_OSPM1_IDX0` as RUNNING,
     * set expected `SCMI_CLOCK_OSPM0_IDX3` as RUNNING,
     * set expected `SCMI_CLOCK_OSPM1_IDX0` as STOPPED,
     * ref_count == 2 and expected ref_count == 1
     */
    setup_agent_state_table(
        FAKE_SCMI_AGENT_IDX_OSPM0,
        SCMI_CLOCK_OSPM0_IDX3,
        MOD_CLOCK_STATE_RUNNING);
    setup_agent_state_table(
        FAKE_SCMI_AGENT_IDX_OSPM1,
        SCMI_CLOCK_OSPM1_IDX0,
        MOD_CLOCK_STATE_RUNNING);
    setup_ref_count_table(CLOCK_DEV_IDX_FAKE3, 2);

    setup_expected_ospm0_state_table(
        SCMI_CLOCK_OSPM0_IDX3,
        MOD_CLOCK_STATE_RUNNING);
    setup_expected_ospm1_state_table(
        SCMI_CLOCK_OSPM1_IDX0,
        MOD_CLOCK_STATE_STOPPED);
    setup_expected_ref_count_table(CLOCK_DEV_IDX_FAKE3, 1);

    fwk_id_get_element_idx_ExpectAnyArgsAndReturn(CLOCK_DEV_IDX_FAKE3);

    status = mod_scmi_clock_state_update(
        FAKE_SCMI_AGENT_IDX_OSPM1,
        SCMI_CLOCK_OSPM1_IDX0,
        MOD_CLOCK_STATE_STOPPED);
    TEST_ASSERT_EQUAL(FWK_SUCCESS, status);

    assert_ospm0_clock_state_meets_expectations();
    assert_ospm1_clock_state_meets_expectations();
    assert_ref_count_meets_expectations();
}

void test_mod_scmi_clock_state_update_ref_count_1_stopped(void)
{
    int status;

    /*
     * Set SCMI clock 0 from `FAKE_SCMI_AGENT_IDX_OSPM0` as RUNNING,
     * set expected SCMI clock 0 from `FAKE_SCMI_AGENT_IDX_OSPM0` as STOPPED,
     * ref_count == 1 and expected ref_count == 0
     */
    setup_agent_state_table(
        FAKE_SCMI_AGENT_IDX_OSPM0,
        SCMI_CLOCK_OSPM0_IDX0,
        MOD_CLOCK_STATE_RUNNING);
    setup_expected_ref_count_table(CLOCK_DEV_IDX_FAKE0, 0);

    setup_expected_ospm0_state_table(
        SCMI_CLOCK_OSPM0_IDX0,
        MOD_CLOCK_STATE_STOPPED);
    setup_ref_count_table(CLOCK_DEV_IDX_FAKE0, 1);

    fwk_id_get_element_idx_ExpectAnyArgsAndReturn(CLOCK_DEV_IDX_FAKE0);

    status = mod_scmi_clock_state_update(
        FAKE_SCMI_AGENT_IDX_OSPM0,
        SCMI_CLOCK_OSPM0_IDX0,
        MOD_CLOCK_STATE_STOPPED);
    TEST_ASSERT_EQUAL(FWK_SUCCESS, status);

    assert_ospm0_clock_state_meets_expectations();
    assert_ref_count_meets_expectations();
}

int clock_attributes_invalid_clock_id_callback(
    fwk_id_t service_id,
    const void *payload,
    size_t size,
    int NumCalls)
{
    struct scmi_clock_attributes_p2a *return_values;
    return_values = (struct scmi_clock_attributes_p2a *)payload;

    TEST_ASSERT_EQUAL((int32_t)SCMI_NOT_FOUND, return_values->status);

    return FWK_SUCCESS;
}

void test_mod_scmi_clock_attributes_handler_invalid_clock_id(void)
{
    int status;
    uint32_t agent_id = FAKE_SCMI_AGENT_IDX_OSPM0;

    fwk_id_t service_id =
        FWK_ID_ELEMENT_INIT(FAKE_MODULE_IDX, FAKE_SCMI_AGENT_IDX_OSPM0);

    struct scmi_clock_attributes_a2p payload = {
        .clock_id = SCMI_CLOCK_OSPM0_COUNT,
    };

    mod_scmi_from_protocol_api_scmi_frame_validation_ExpectAnyArgsAndReturn(
        FWK_SUCCESS);

    mod_scmi_from_protocol_api_get_agent_id_ExpectAnyArgsAndReturn(FWK_SUCCESS);
    mod_scmi_from_protocol_api_get_agent_id_ReturnThruPtr_agent_id(&agent_id);

    mod_scmi_from_protocol_api_respond_Stub(
        clock_attributes_invalid_clock_id_callback);

    status = scmi_clock_message_handler(
        (fwk_id_t)MOD_SCMI_PROTOCOL_ID_CLOCK,
        service_id,
        (const uint32_t *)&payload,
        payload_size_table[MOD_SCMI_CLOCK_ATTRIBUTES],
        MOD_SCMI_CLOCK_ATTRIBUTES);

    TEST_ASSERT_EQUAL(FWK_E_RANGE, status);
}

int clock_attributes_get_state_callback(
    fwk_id_t service_id,
    const void *payload,
    size_t size,
    int NumCalls)
{
    uint32_t expected_attributes;
    struct scmi_clock_attributes_p2a *return_values;
    return_values = (struct scmi_clock_attributes_p2a *)payload;

    bool extended_name = true, notify_rate = false, notify_requested_rate = false;

    expected_attributes = SET_SCMI_CLOCK_ATTRIBUTES(MOD_CLOCK_STATE_RUNNING,
                                                    extended_name,
                                                    notify_rate,
                                                    notify_requested_rate);

    TEST_ASSERT_EQUAL((int32_t)SCMI_SUCCESS, return_values->status);
    TEST_ASSERT_EQUAL(expected_attributes, return_values->attributes);

    return FWK_SUCCESS;
}

void test_mod_scmi_clock_attributes_handler_get_state(void)
{
    int status;
    uint32_t agent_id = FAKE_SCMI_AGENT_IDX_OSPM1;

    fwk_id_t service_id =
        FWK_ID_ELEMENT_INIT(FAKE_MODULE_IDX, FAKE_SCMI_AGENT_IDX_OSPM0);

    struct scmi_clock_attributes_a2p payload = {
        .clock_id = SCMI_CLOCK_OSPM1_IDX0,
    };

    setup_agent_state_table(
        FAKE_SCMI_AGENT_IDX_OSPM1,
        SCMI_CLOCK_OSPM1_IDX0,
        MOD_CLOCK_STATE_RUNNING);

    mod_scmi_from_protocol_api_scmi_frame_validation_ExpectAnyArgsAndReturn(
        FWK_SUCCESS);

    mod_scmi_from_protocol_api_get_agent_id_ExpectAnyArgsAndReturn(FWK_SUCCESS);
    mod_scmi_from_protocol_api_get_agent_id_ReturnThruPtr_agent_id(&agent_id);

    fwk_module_is_valid_element_id_ExpectAnyArgsAndReturn(true);
    fwk_id_get_element_idx_Stub(get_element_idx_callback);

#ifdef BUILD_HAS_AGENT_LOGICAL_DOMAIN
    mod_scmi_from_protocol_api_get_agent_id_ExpectAnyArgsAndReturn(FWK_SUCCESS);
    mod_scmi_from_protocol_api_get_agent_id_ReturnThruPtr_agent_id(&agent_id);

    fwk_module_get_element_name_ExpectAnyArgsAndReturn("");

    mod_scmi_from_protocol_api_respond_Stub(
        clock_attributes_get_state_callback);
#else
    fwk_id_is_equal_ExpectAnyArgsAndReturn(true);
    __fwk_put_event_ExpectAnyArgsAndReturn(FWK_SUCCESS);
#endif

    fwk_id_get_element_idx_Stub(get_element_idx_callback);

    status = scmi_clock_message_handler(
        (fwk_id_t)MOD_SCMI_PROTOCOL_ID_CLOCK,
        service_id,
        (const uint32_t *)&payload,
        payload_size_table[MOD_SCMI_CLOCK_ATTRIBUTES],
        MOD_SCMI_CLOCK_ATTRIBUTES);

    TEST_ASSERT_EQUAL(FWK_SUCCESS, status);
}

int version_handler_respond_callback(
    fwk_id_t service_id,
    const void *payload,
    size_t size,
    int NumCalls)
{
    struct scmi_protocol_version_p2a *return_values;
    return_values = (struct scmi_protocol_version_p2a *)payload;

    TEST_ASSERT_EQUAL((int32_t)SCMI_SUCCESS, return_values->status);
    TEST_ASSERT_EQUAL(SCMI_PROTOCOL_VERSION_CLOCK, return_values->version);

    return FWK_SUCCESS;
}

void test_mod_scmi_clock_protocol_version_handler(void)
{
    int status;
    fwk_id_t service_id =
        FWK_ID_ELEMENT_INIT(FAKE_MODULE_IDX, FAKE_SCMI_AGENT_IDX_PSCI);

    uint32_t payload = 0;

    mod_scmi_from_protocol_api_scmi_frame_validation_ExpectAnyArgsAndReturn(
        SCMI_SUCCESS);
    mod_scmi_from_protocol_api_respond_Stub(version_handler_respond_callback);

    status = scmi_clock_message_handler(
        (fwk_id_t)MOD_SCMI_PROTOCOL_ID_CLOCK,
        service_id,
        (const uint32_t *)&payload,
        payload_size_table[MOD_SCMI_PROTOCOL_VERSION],
        MOD_SCMI_PROTOCOL_VERSION);

    TEST_ASSERT_EQUAL(FWK_SUCCESS, status);
}

int protocol_attributes_handler_respond_callback(
    fwk_id_t service_id,
    const void *payload,
    size_t size,
    int NumCalls)
{
    struct scmi_protocol_attributes_p2a *return_values;
    return_values = (struct scmi_protocol_attributes_p2a *)payload;

    TEST_ASSERT_EQUAL((int32_t)SCMI_SUCCESS, return_values->status);
    TEST_ASSERT_EQUAL(
        SCMI_CLOCK_PROTOCOL_ATTRIBUTES(scmi_clock_ctx.max_pending_transactions,
                                       SCMI_CLOCK_OSPM1_COUNT),
        return_values->attributes);

    return FWK_SUCCESS;
}

void test_mod_scmi_clock_protocol_attributes_handler(void)
{
    int status;
    uint32_t agent_id = FAKE_SCMI_AGENT_IDX_OSPM1;
    fwk_id_t service_id =
        FWK_ID_ELEMENT_INIT(FAKE_MODULE_IDX, agent_id);

    uint32_t payload = 0;

    mod_scmi_from_protocol_api_get_agent_id_ExpectAnyArgsAndReturn(
        FWK_SUCCESS);
    mod_scmi_from_protocol_api_get_agent_id_ReturnThruPtr_agent_id(&agent_id);
    mod_scmi_from_protocol_api_scmi_frame_validation_ExpectAnyArgsAndReturn(
        SCMI_SUCCESS);
    mod_scmi_from_protocol_api_respond_Stub(
        protocol_attributes_handler_respond_callback);

    status = scmi_clock_message_handler(
        (fwk_id_t)MOD_SCMI_PROTOCOL_ID_CLOCK,
        service_id,
        (const uint32_t *)&payload,
        payload_size_table[MOD_SCMI_PROTOCOL_ATTRIBUTES],
        MOD_SCMI_PROTOCOL_ATTRIBUTES);

    TEST_ASSERT_EQUAL(FWK_SUCCESS, status);
}

int protocol_message_attributes_handler_respond_callback_notify_rate(
    fwk_id_t service_id,
    const void *payload,
    size_t size,
    int NumCalls)
{
    struct scmi_protocol_message_attributes_p2a *return_values;
    return_values = (struct scmi_protocol_message_attributes_p2a *)payload;

#ifdef BUILD_HAS_SCMI_NOTIFICATIONS
    TEST_ASSERT_EQUAL((int32_t)SCMI_SUCCESS, return_values->status);
#else
    TEST_ASSERT_EQUAL((int32_t)SCMI_NOT_FOUND, return_values->status);
#endif

    return FWK_SUCCESS;
}

void test_protocol_message_attributes_handler_notify_rate(void)
{
    int status;
    fwk_id_t service_id =
        FWK_ID_ELEMENT_INIT(FAKE_MODULE_IDX, FAKE_SCMI_AGENT_IDX_OSPM1);

    uint32_t payload = MOD_SCMI_CLOCK_RATE_NOTIFY;

    mod_scmi_from_protocol_api_scmi_frame_validation_ExpectAnyArgsAndReturn(
        SCMI_SUCCESS);
    mod_scmi_from_protocol_api_respond_Stub(
        protocol_message_attributes_handler_respond_callback_notify_rate);

    status = scmi_clock_message_handler(
        (fwk_id_t)MOD_SCMI_PROTOCOL_ID_CLOCK,
        service_id,
        (const uint32_t *)&payload,
        payload_size_table[MOD_SCMI_PROTOCOL_MESSAGE_ATTRIBUTES],
        MOD_SCMI_PROTOCOL_MESSAGE_ATTRIBUTES);

    TEST_ASSERT_EQUAL(FWK_SUCCESS, status);
}

void test_protocol_message_attributes_handler_notify_rate_requested(void)
{
    int status;
    fwk_id_t service_id =
        FWK_ID_ELEMENT_INIT(FAKE_MODULE_IDX, FAKE_SCMI_AGENT_IDX_OSPM1);

    uint32_t payload = MOD_SCMI_CLOCK_RATE_CHANGE_REQUESTED_NOTIFY;

    mod_scmi_from_protocol_api_scmi_frame_validation_ExpectAnyArgsAndReturn(
        SCMI_SUCCESS);
    mod_scmi_from_protocol_api_respond_Stub(
        protocol_message_attributes_handler_respond_callback_notify_rate);

    status = scmi_clock_message_handler(
        (fwk_id_t)MOD_SCMI_PROTOCOL_ID_CLOCK,
        service_id,
        (const uint32_t *)&payload,
        payload_size_table[MOD_SCMI_PROTOCOL_MESSAGE_ATTRIBUTES],
        MOD_SCMI_PROTOCOL_MESSAGE_ATTRIBUTES);

    TEST_ASSERT_EQUAL(FWK_SUCCESS, status);
}

int get_state_callback_success(
    fwk_id_t service_id,
    const void *payload,
    size_t size,
    int NumCalls
)
{
    uint32_t expected_attributes;
    struct scmi_clock_attributes_p2a *return_values;
    return_values = (struct scmi_clock_attributes_p2a *)payload;
    bool clock_enable, extended_name, notify_rate, notify_requested_rate;

    clock_enable = true;
    extended_name = true;

    TEST_ASSERT_EQUAL((int32_t)SCMI_SUCCESS, return_values->status);
    TEST_ASSERT_EQUAL_STRING_LEN(mock_clock_name, return_values->clock_name,
                                 SCMI_CLOCK_NAME_LENGTH-1);
    TEST_ASSERT_EQUAL(0, return_values->clock_enable_delay);

#ifdef BUILD_HAS_SCMI_NOTIFICATIONS
    notify_rate = true;
    notify_requested_rate = true;
#else
    notify_rate = false;
    notify_requested_rate = false;
#endif

    expected_attributes = SET_SCMI_CLOCK_ATTRIBUTES(clock_enable,
                                                extended_name,
                                                notify_rate,
                                                notify_requested_rate);
    TEST_ASSERT_EQUAL(expected_attributes, return_values->attributes);

    return FWK_SUCCESS;
}

void test_process_request_event_get_state(void)
{
    int status;
    uint32_t agent_id = FAKE_SCMI_AGENT_IDX_OSPM0;
    struct fwk_event event;
    struct scmi_clock_event_request_params *params;
    enum mod_clock_state expected_state = MOD_CLOCK_STATE_RUNNING;

    params = (struct scmi_clock_event_request_params *)event.params;
    params->clock_dev_id = (fwk_id_t) FWK_ID_ELEMENT_INIT(FWK_MODULE_IDX_CLOCK, CLOCK_DEV_IDX_FAKE1);

    fwk_id_get_event_idx_ExpectAnyArgsAndReturn(
        SCMI_CLOCK_EVENT_IDX_CLOCK_ATTRIBUTES);

    mod_scmi_from_protocol_api_get_agent_id_ExpectAnyArgsAndReturn(
        FWK_SUCCESS);
    mod_scmi_from_protocol_api_get_agent_id_ReturnThruPtr_agent_id(&agent_id);

    mod_clock_api_get_state_ExpectAnyArgsAndReturn(FWK_SUCCESS);
    mod_clock_api_get_state_ReturnThruPtr_state(&expected_state);

    fwk_module_get_element_name_ExpectAnyArgsAndReturn(mock_clock_name);

    mod_scmi_from_protocol_api_get_agent_id_ExpectAnyArgsAndReturn(
        FWK_SUCCESS);
    mod_scmi_from_protocol_api_get_agent_id_ReturnThruPtr_agent_id(&agent_id);

    fwk_id_get_element_idx_Stub(get_element_idx_callback);

    mod_scmi_from_protocol_api_respond_Stub(get_state_callback_success);

    status = process_request_event(&event);

    TEST_ASSERT_EQUAL(status, FWK_SUCCESS);
}

int get_extended_name_callback_success(
    fwk_id_t service_id,
    const void *payload,
    size_t size,
    int NumCalls
)
{
    struct scmi_clock_name_get_p2a *return_values;
    return_values = (struct scmi_clock_name_get_p2a *)payload;

    TEST_ASSERT_EQUAL((int32_t)SCMI_SUCCESS, return_values->status);
    TEST_ASSERT_EQUAL(0, return_values->flags);
    TEST_ASSERT_EQUAL_STRING_LEN(mock_clock_name, return_values->clock_extended_name,
                                 SCMI_CLOCK_EXTENDED_NAME_LENGTH-1);

    return FWK_SUCCESS;
}

void test_process_request_event_get_clock_extended_name(void)
{
    int status;
    uint32_t agent_id = FAKE_SCMI_AGENT_IDX_OSPM0;
    struct fwk_event event;

    fwk_id_get_element_idx_ExpectAnyArgsAndReturn(SCMI_CLOCK_OSPM0_IDX1);
    fwk_id_get_event_idx_ExpectAnyArgsAndReturn(SCMI_CLOCK_EVENT_IDX_GET_NAME);
    mod_scmi_from_protocol_api_get_agent_id_ExpectAnyArgsAndReturn(
        FWK_SUCCESS);
    mod_scmi_from_protocol_api_get_agent_id_ReturnThruPtr_agent_id(&agent_id);
    fwk_module_get_element_name_ExpectAnyArgsAndReturn(mock_clock_name);

    mod_scmi_from_protocol_api_respond_Stub(
        get_extended_name_callback_success);

    status = process_request_event(&event);

    TEST_ASSERT_EQUAL(status, FWK_SUCCESS);
}

void test_scmi_clock_name_get_handler_success(void)
{
    int status;
    uint32_t agent_id = FAKE_SCMI_AGENT_IDX_OSPM0;

    fwk_id_t service_id =
        FWK_ID_ELEMENT_INIT(FAKE_MODULE_IDX, FAKE_SCMI_AGENT_IDX_OSPM0);

    struct scmi_clock_name_get_a2p payload = {
        .clock_id = SCMI_CLOCK_OSPM0_IDX1,
    };

    mod_scmi_from_protocol_api_scmi_frame_validation_ExpectAnyArgsAndReturn(
            FWK_SUCCESS);

    mod_scmi_from_protocol_api_get_agent_id_ExpectAnyArgsAndReturn(FWK_SUCCESS);
    mod_scmi_from_protocol_api_get_agent_id_ReturnThruPtr_agent_id(&agent_id);

    fwk_module_is_valid_element_id_ExpectAnyArgsAndReturn(true);

    fwk_id_get_element_idx_ExpectAnyArgsAndReturn(CLOCK_DEV_IDX_FAKE1);
    fwk_id_is_equal_ExpectAnyArgsAndReturn(true);

    __fwk_put_event_ExpectAnyArgsAndReturn(FWK_SUCCESS);

    status = scmi_clock_message_handler(
        (fwk_id_t)MOD_SCMI_PROTOCOL_ID_CLOCK,
        service_id,
        (const uint32_t *)&payload,
        payload_size_table[MOD_SCMI_CLOCK_NAME_GET],
        MOD_SCMI_CLOCK_NAME_GET);

    TEST_ASSERT_EQUAL(FWK_SUCCESS, status);
}

int clock_has_extended_name_callback(
    fwk_id_t service_id,
    const void *payload,
    size_t size,
    int NumCalls)
{
    struct scmi_clock_name_get_p2a *return_values;
    return_values = (struct scmi_clock_name_get_p2a *)payload;

    TEST_ASSERT_EQUAL((int32_t)SCMI_NOT_SUPPORTED, return_values->status);

    return FWK_SUCCESS;
}

void test_scmi_clock_name_get_handler_extended_name_not_supported(void)
{
    int status;
    uint32_t agent_id = FAKE_SCMI_AGENT_IDX_OSPM0;

    fwk_id_t service_id =
        FWK_ID_ELEMENT_INIT(FAKE_MODULE_IDX, FAKE_SCMI_AGENT_IDX_OSPM0);

    struct scmi_clock_name_get_a2p payload = {
        .clock_id = SCMI_CLOCK_OSPM0_IDX0,
    };

    mod_scmi_from_protocol_api_scmi_frame_validation_ExpectAnyArgsAndReturn(
            FWK_SUCCESS);

    mod_scmi_from_protocol_api_get_agent_id_ExpectAnyArgsAndReturn(FWK_SUCCESS);
    mod_scmi_from_protocol_api_get_agent_id_ReturnThruPtr_agent_id(&agent_id);

    fwk_module_is_valid_element_id_ExpectAnyArgsAndReturn(true);

    mod_scmi_from_protocol_api_respond_Stub(
        clock_has_extended_name_callback);

    status = scmi_clock_message_handler(
        (fwk_id_t)MOD_SCMI_PROTOCOL_ID_CLOCK,
        service_id,
        (const uint32_t *)&payload,
        payload_size_table[MOD_SCMI_CLOCK_NAME_GET],
        MOD_SCMI_CLOCK_NAME_GET);

    TEST_ASSERT_EQUAL(FWK_SUCCESS, status);
}

int rate_notify_invalid_param_callback(
    fwk_id_t service_id,
    const void *payload,
    size_t size,
    int NumCalls
)
{
    struct scmi_clock_rate_notify_p2a *return_values;
    return_values = (struct scmi_clock_rate_notify_p2a *)payload;

    TEST_ASSERT_EQUAL((int32_t)SCMI_NOT_FOUND, return_values->status);

    return FWK_SUCCESS;
}

void test_clock_rate_changed_notify_handler_invalid_clock_id(void)
{
    int status;
    unsigned int agent_id = FAKE_SCMI_AGENT_IDX_OSPM0;
    fwk_id_t service_id =
        FWK_ID_ELEMENT_INIT(FAKE_MODULE_IDX, agent_id);

    struct scmi_clock_rate_notify_a2p payload = {
        .clock_id = SCMI_CLOCK_OSPM0_COUNT,
        .notify_enable = true,
    };

    mod_scmi_from_protocol_api_get_agent_id_ExpectAnyArgsAndReturn(
        FWK_SUCCESS);
    mod_scmi_from_protocol_api_get_agent_id_ReturnThruPtr_agent_id(&agent_id);

    mod_scmi_from_protocol_api_scmi_frame_validation_ExpectAnyArgsAndReturn(
        FWK_SUCCESS);

    mod_scmi_from_protocol_api_respond_Stub(rate_notify_invalid_param_callback);

    status = scmi_clock_message_handler(
        (fwk_id_t)MOD_SCMI_PROTOCOL_ID_CLOCK,
        service_id,
        (const uint32_t *)&payload,
        payload_size_table[MOD_SCMI_CLOCK_RATE_NOTIFY],
        MOD_SCMI_CLOCK_RATE_NOTIFY);

    TEST_ASSERT_EQUAL(FWK_E_RANGE, status);
}

int scmi_notification_add_subscriber_callback(
    unsigned int operation_id,
    unsigned int resource_id,
    unsigned int subscriber_id,
    fwk_id_t service_id,
    int NumCalls
)
{
    fwk_id_t expected_service_id =
        FWK_ID_ELEMENT_INIT(FAKE_MODULE_IDX, FAKE_SCMI_AGENT_IDX_OSPM0);

    TEST_ASSERT_EQUAL(FAKE_SCMI_AGENT_IDX_OSPM0, subscriber_id);
    TEST_ASSERT_EQUAL(CLOCK_DEV_IDX_FAKE1, resource_id);
    TEST_ASSERT_EQUAL(SCMI_CLOCK_RATE_CHANGED, operation_id);
    TEST_ASSERT_EQUAL(expected_service_id.value, service_id.value);

    return FWK_SUCCESS;
}

void test_clock_rate_changed_notify_handler_add_subscriber(void)
{
    int status;
    unsigned int agent_id = FAKE_SCMI_AGENT_IDX_OSPM0;
    fwk_id_t service_id =
        FWK_ID_ELEMENT_INIT(FAKE_MODULE_IDX, agent_id);

    struct scmi_clock_rate_notify_a2p payload = {
        .clock_id = SCMI_CLOCK_OSPM0_IDX1,
        .notify_enable = true,
    };

    mod_scmi_from_protocol_api_get_agent_id_ExpectAnyArgsAndReturn(
        FWK_SUCCESS);
    mod_scmi_from_protocol_api_get_agent_id_ReturnThruPtr_agent_id(&agent_id);

    fwk_module_is_valid_element_id_ExpectAnyArgsAndReturn(true);

    mod_scmi_from_protocol_api_scmi_frame_validation_ExpectAnyArgsAndReturn(
        FWK_SUCCESS);

    fwk_id_get_element_idx_ExpectAnyArgsAndReturn(CLOCK_DEV_IDX_FAKE1);

    mod_scmi_from_protocol_api_respond_ExpectAnyArgsAndReturn(FWK_SUCCESS);

    status = scmi_clock_message_handler(
        (fwk_id_t)MOD_SCMI_PROTOCOL_ID_CLOCK,
        service_id,
        (const uint32_t *)&payload,
        payload_size_table[MOD_SCMI_CLOCK_RATE_NOTIFY],
        MOD_SCMI_CLOCK_RATE_NOTIFY);

    TEST_ASSERT_EQUAL(FWK_SUCCESS, status);
}

int scmi_notification_remove_subscriber_callback(
    unsigned int operation_id,
    unsigned int resource_id,
    unsigned int subscriber_id,
    fwk_id_t service_id,
    int NumCalls
)
{
    fwk_id_t expected_service_id = FWK_ID_NONE;
    TEST_ASSERT_EQUAL(FAKE_SCMI_AGENT_IDX_OSPM0, subscriber_id);
    TEST_ASSERT_EQUAL(SCMI_CLOCK_OSPM0_IDX1, resource_id);
    TEST_ASSERT_EQUAL(MOD_SCMI_CLOCK_RATE_NOTIFY, operation_id);
    TEST_ASSERT_EQUAL(expected_service_id.value, service_id.value);

    return FWK_SUCCESS;
}

void test_clock_rate_changed_notify_handler_remove_subscriber(void)
{
    int status;
    unsigned int agent_id = FAKE_SCMI_AGENT_IDX_OSPM0;
    fwk_id_t service_id =
        FWK_ID_ELEMENT_INIT(FAKE_MODULE_IDX, agent_id);

    struct scmi_clock_rate_notify_a2p payload = {
        .clock_id = SCMI_CLOCK_OSPM0_IDX1,
        .notify_enable = false,
    };

    mod_scmi_from_protocol_api_get_agent_id_ExpectAnyArgsAndReturn(
        FWK_SUCCESS);
    mod_scmi_from_protocol_api_get_agent_id_ReturnThruPtr_agent_id(&agent_id);
    fwk_module_is_valid_element_id_ExpectAnyArgsAndReturn(true);

    mod_scmi_from_protocol_api_scmi_frame_validation_ExpectAnyArgsAndReturn(
        FWK_SUCCESS);

    fwk_id_get_element_idx_ExpectAnyArgsAndReturn(CLOCK_DEV_IDX_FAKE1);

    mod_scmi_from_protocol_api_respond_ExpectAnyArgsAndReturn(FWK_SUCCESS);

    status = scmi_clock_message_handler(
        (fwk_id_t)MOD_SCMI_PROTOCOL_ID_CLOCK,
        service_id,
        (const uint32_t *)&payload,
        payload_size_table[MOD_SCMI_CLOCK_RATE_NOTIFY],
        MOD_SCMI_CLOCK_RATE_NOTIFY);

    TEST_ASSERT_EQUAL(FWK_SUCCESS, status);
}

void test_clock_rate_change_requested_notify_handler_add_subscriber(void)
{
    int status;
    unsigned int agent_id = FAKE_SCMI_AGENT_IDX_OSPM0;
    fwk_id_t service_id =
        FWK_ID_ELEMENT_INIT(FAKE_MODULE_IDX, agent_id);

    struct scmi_clock_rate_notify_a2p payload = {
        .clock_id = SCMI_CLOCK_OSPM0_IDX1,
        .notify_enable = true,
    };

    mod_scmi_from_protocol_api_get_agent_id_ExpectAnyArgsAndReturn(
        FWK_SUCCESS);
    mod_scmi_from_protocol_api_get_agent_id_ReturnThruPtr_agent_id(&agent_id);
    fwk_module_is_valid_element_id_ExpectAnyArgsAndReturn(true);

    mod_scmi_from_protocol_api_scmi_frame_validation_ExpectAnyArgsAndReturn(
        FWK_SUCCESS);

    fwk_id_get_element_idx_ExpectAnyArgsAndReturn(CLOCK_DEV_IDX_FAKE1);

    mod_scmi_from_protocol_api_respond_ExpectAnyArgsAndReturn(FWK_SUCCESS);

    status = scmi_clock_message_handler(
        (fwk_id_t)MOD_SCMI_PROTOCOL_ID_CLOCK,
        service_id,
        (const uint32_t *)&payload,
        payload_size_table[MOD_SCMI_CLOCK_RATE_CHANGE_REQUESTED_NOTIFY],
        MOD_SCMI_CLOCK_RATE_CHANGE_REQUESTED_NOTIFY);

    TEST_ASSERT_EQUAL(FWK_SUCCESS, status);
}

void test_clock_rate_change_requested_notify_handler_remove_subscriber(void)
{
    int status;
    unsigned int agent_id = FAKE_SCMI_AGENT_IDX_OSPM0;
    fwk_id_t service_id =
        FWK_ID_ELEMENT_INIT(FAKE_MODULE_IDX, agent_id);

    struct scmi_clock_rate_notify_a2p payload = {
        .clock_id = SCMI_CLOCK_OSPM0_IDX1,
        .notify_enable = false,
    };

    mod_scmi_from_protocol_api_get_agent_id_ExpectAnyArgsAndReturn(
        FWK_SUCCESS);
    mod_scmi_from_protocol_api_get_agent_id_ReturnThruPtr_agent_id(&agent_id);
    fwk_module_is_valid_element_id_ExpectAnyArgsAndReturn(true);

    mod_scmi_from_protocol_api_scmi_frame_validation_ExpectAnyArgsAndReturn(
        FWK_SUCCESS);

    fwk_id_get_element_idx_ExpectAnyArgsAndReturn(CLOCK_DEV_IDX_FAKE1);

    mod_scmi_from_protocol_api_respond_ExpectAnyArgsAndReturn(FWK_SUCCESS);

    status = scmi_clock_message_handler(
        (fwk_id_t)MOD_SCMI_PROTOCOL_ID_CLOCK,
        service_id,
        (const uint32_t *)&payload,
        payload_size_table[MOD_SCMI_CLOCK_RATE_CHANGE_REQUESTED_NOTIFY],
        MOD_SCMI_CLOCK_RATE_CHANGE_REQUESTED_NOTIFY);

    TEST_ASSERT_EQUAL(FWK_SUCCESS, status);
}

#ifdef BUILD_HAS_SCMI_NOTIFICATIONS
void scmi_notify_rate_changed_callback(fwk_id_t id, int protocol_id, int message_id,
    const void *payload, size_t size, int NumCalls)
{
    struct scmi_clock_rate_notification_message_p2a *message = (struct scmi_clock_rate_notification_message_p2a *)payload;
    fwk_id_t expected_service_id = FWK_ID_ELEMENT(FAKE_MODULE_IDX, FAKE_SCMI_AGENT_IDX_OSPM0);

    TEST_ASSERT_EQUAL(expected_service_id.value, id.value);
    TEST_ASSERT_EQUAL(MOD_SCMI_PROTOCOL_ID_CLOCK, protocol_id);
    TEST_ASSERT_EQUAL(SCMI_CLOCK_RATE_CHANGED, message_id);
    TEST_ASSERT_EQUAL(FAKE_SCMI_AGENT_IDX_OSPM1, message->agent_id);
    TEST_ASSERT_EQUAL(SCMI_CLOCK_OSPM0_IDX3, message->clock_id);
    TEST_ASSERT_EQUAL(0x0000000A, message->rate[0]);
    TEST_ASSERT_EQUAL(0x0000000B, message->rate[1]);
}

void test_mod_scmi_clock_process_notification_rate_changed(void)
{
    int status;
    unsigned int agent_id, clock_idx;
    agent_id = FAKE_SCMI_AGENT_IDX_OSPM1;
    clock_idx = CLOCK_DEV_IDX_FAKE3;
    fwk_id_t clock_id = FWK_ID_ELEMENT(FWK_MODULE_IDX_CLOCK, clock_idx);

    /* In this test case,
     * subscriber = OSPM0 agent
     * resource = SCMI_CLOCK_OSPM0_IDX3/CLOCK_DEV_IDX_FAKE3
     * operation = Clock rate changed notification
     */
    scmi_clock_ctx.notification_table[SCMI_CLOCK_RATE_CHANGED][CLOCK_DEV_IDX_FAKE3][FAKE_SCMI_AGENT_IDX_OSPM0] =
                    FWK_ID_ELEMENT(FAKE_MODULE_IDX, FAKE_SCMI_AGENT_IDX_OSPM0);


    struct fwk_event notification_event, resp_event;
    struct mod_clock_notification_params *notification_params =
        ((struct mod_clock_notification_params *)notification_event.params);

    notification_event.id = FWK_ID_NOTIFICATION(
        FWK_MODULE_IDX_CLOCK, MOD_CLOCK_NOTIFICATION_IDX_RATE_CHANGED);
    notification_params->clock_id = clock_id;
    notification_params->requester_id = agent_id;
    notification_params->rate = (uint64_t) 0x0000000B0000000A;

    fwk_id_get_notification_idx_ExpectAnyArgsAndReturn(
        MOD_CLOCK_NOTIFICATION_IDX_RATE_CHANGED);

    fwk_id_get_element_idx_Stub(get_element_idx_callback);
    fwk_id_is_equal_ExpectAnyArgsAndReturn(true);
    fwk_id_is_equal_ExpectAnyArgsAndReturn(false);
    fwk_id_is_equal_ExpectAnyArgsAndReturn(true);

    mod_scmi_from_protocol_api_notify_Stub(scmi_notify_rate_changed_callback);

    status = scmi_clock_process_notification(&notification_event, &resp_event);

    TEST_ASSERT_EQUAL(status, FWK_SUCCESS);
}

void test_mod_scmi_clock_process_notification_rate_change_requested(void)
{
    int status;
    unsigned int agent_id, clock_idx;
    agent_id = FAKE_SCMI_AGENT_IDX_OSPM1;
    clock_idx = CLOCK_DEV_IDX_FAKE3;
    fwk_id_t clock_id = FWK_ID_ELEMENT(FWK_MODULE_IDX_CLOCK, clock_idx);
    struct fwk_event notification_event, resp_event;
    struct mod_clock_notification_params *notification_params =
        ((struct mod_clock_notification_params *)notification_event.params);

    notification_event.id = FWK_ID_NOTIFICATION(
        FWK_MODULE_IDX_CLOCK,
        MOD_CLOCK_NOTIFICATION_IDX_RATE_CHANGE_REQUESTED);
    notification_params->clock_id = clock_id;
    notification_params->requester_id = agent_id;
    notification_params->rate = (uint64_t) 0x0000000D0000000C;

    fwk_id_get_notification_idx_ExpectAnyArgsAndReturn(
        MOD_CLOCK_NOTIFICATION_IDX_RATE_CHANGE_REQUESTED);

    fwk_id_get_element_idx_Stub(get_element_idx_callback);
    fwk_id_is_equal_ExpectAnyArgsAndReturn(true);
    fwk_id_is_equal_ExpectAnyArgsAndReturn(true);
    fwk_id_is_equal_ExpectAnyArgsAndReturn(true);

    status = scmi_clock_process_notification(&notification_event, &resp_event);

    TEST_ASSERT_EQUAL(status, FWK_SUCCESS);
}

void test_mod_scmi_clock_start()
{
    int status;
    fwk_id_t module_id = FWK_ID_MODULE_INIT(FWK_MODULE_IDX_SCMI_CLOCK);

    fwk_notification_subscribe_ExpectAndReturn(
        mod_clock_notification_id_rate_changed,
        FWK_ID_MODULE(FWK_MODULE_IDX_CLOCK),
        module_id,
        FWK_SUCCESS);

    fwk_notification_subscribe_ExpectAndReturn(
        mod_clock_notification_id_rate_change_requested,
        FWK_ID_MODULE(FWK_MODULE_IDX_CLOCK),
        module_id,
        FWK_SUCCESS);

    status = scmi_clock_start(module_id);

    TEST_ASSERT_EQUAL(status, FWK_SUCCESS);
}

void test_mod_scmi_clock_bind(void)
{
    int status;
    fwk_id_t mod_scmi_clock_id = FWK_ID_MODULE_INIT(FWK_MODULE_IDX_SCMI_CLOCK);
    fwk_id_t mod_scmi_id = FWK_ID_MODULE(FWK_MODULE_IDX_SCMI);
    fwk_id_t mod_clock_id = FWK_ID_MODULE(FWK_MODULE_IDX_CLOCK);
    fwk_id_is_type_ExpectAnyArgsAndReturn(false);

    fwk_id_t mod_scmi_api_protocol =
        FWK_ID_API(FWK_MODULE_IDX_SCMI,
                   MOD_SCMI_API_IDX_PROTOCOL);

    fwk_id_t mod_clock_api =
        FWK_ID_API(FWK_MODULE_IDX_CLOCK,
                   0);

    fwk_module_bind_ExpectAndReturn(mod_scmi_id,
                                    mod_scmi_api_protocol,
                                    &scmi_clock_ctx.scmi_api,
                                    FWK_SUCCESS);

    fwk_module_bind_ExpectAndReturn(mod_clock_id,
                                    mod_clock_api,
                                    &scmi_clock_ctx.clock_api,
                                    FWK_SUCCESS);

    status = scmi_clock_bind(mod_scmi_clock_id, 0);

    TEST_ASSERT_EQUAL(status, FWK_SUCCESS);
}
#endif

void test_scmi_clock_init(void)
{
    int status;
    uint8_t *dev_clock_ref_count_table_return = (uint8_t *)0xAAAAAAAA;
    size_t phy_device_count = CLOCK_DEV_IDX_COUNT;
    fwk_id_t scmi_clock_module_id = FWK_ID_MODULE(FWK_MODULE_IDX_SCMI_CLOCK);

    fwk_mm_calloc_ExpectAndReturn(FAKE_SCMI_AGENT_IDX_COUNT,
                                  sizeof(struct mod_scmi_clock_agent *),
                                  agent_table);

    fwk_module_get_element_count_ExpectAnyArgsAndReturn(FWK_SUCCESS);
    fwk_module_get_element_count_ReturnThruPtr_mod_elem_count(&phy_device_count);

    fwk_mm_calloc_ExpectAndReturn(CLOCK_DEV_IDX_COUNT,
                                  sizeof(struct clock_operations),
                                  clock_ops_table);

    fwk_mm_calloc_ExpectAndReturn(CLOCK_DEV_IDX_COUNT,
                                  sizeof(uint8_t),
                                  dev_clock_ref_count_table_return);

#ifdef BUILD_HAS_SCMI_NOTIFICATIONS
    fwk_mm_calloc_ExpectAndReturn(SCMI_CLOCK_NOTIFICATION_COUNT,
                                  sizeof(fwk_id_t **),
                                  operation_table);

    for (unsigned int op_idx = 0; op_idx < SCMI_CLOCK_NOTIFICATION_COUNT; op_idx++) {
        fwk_mm_calloc_ExpectAndReturn(scmi_clock_ctx.phy_device_count,
                                      sizeof(fwk_id_t *),
                                      resource_table);

        for (unsigned int res_idx = 0; res_idx < scmi_clock_ctx.phy_device_count; res_idx++) {
            fwk_mm_calloc_ExpectAndReturn(scmi_clock_ctx.agent_count,
                                          sizeof(fwk_id_t),
                                          subscriber_table);
        }
    }

#endif

    status = scmi_clock_init(scmi_clock_module_id, FAKE_SCMI_AGENT_IDX_COUNT, config_scmi_clock.data);

    TEST_ASSERT_EQUAL(MAX_PENDING_TRANSACTION, scmi_clock_ctx.max_pending_transactions);
    TEST_ASSERT_EQUAL(FAKE_SCMI_AGENT_IDX_COUNT, scmi_clock_ctx.agent_count);
    TEST_ASSERT_EQUAL(CLOCK_DEV_IDX_COUNT, scmi_clock_ctx.phy_device_count);

#ifdef BUILD_HAS_SCMI_NOTIFICATIONS
    for (unsigned int op_idx = 0; op_idx < SCMI_CLOCK_NOTIFICATION_COUNT; op_idx++) {
        for (unsigned int res_idx = 0; res_idx < scmi_clock_ctx.phy_device_count; res_idx++) {
            for (unsigned int sub_idx = 0; sub_idx < scmi_clock_ctx.agent_count; sub_idx++) {
                fwk_id_t expected_subscriber_id = FWK_ID_NONE;
                TEST_ASSERT_EQUAL(
                    expected_subscriber_id.value,
                    scmi_clock_ctx.notification_table[op_idx][res_idx][sub_idx].value);
            }
        }
    }
#endif
    TEST_ASSERT_EQUAL(FWK_SUCCESS, status);
}

void test_scmi_clock_element_init(void)
{
    int status;

    memset(dev_clock_ref_count_table, 0, CLOCK_DEV_IDX_COUNT);
    memset(ospm0_state_table, 0, SCMI_CLOCK_OSPM0_COUNT * sizeof(uint8_t));
    memset(ospm1_state_table, 0, SCMI_CLOCK_OSPM1_COUNT * sizeof(uint8_t));

    fwk_id_get_element_idx_Stub(get_element_idx_callback);

    fwk_mm_calloc_ExpectAndReturn(1,
                                  sizeof(struct mod_scmi_clock_agent),
                                  agent_table[FAKE_SCMI_AGENT_IDX_OSPM0]);
    fwk_mm_calloc_ExpectAndReturn(SCMI_CLOCK_OSPM0_COUNT,
                                  sizeof(uint8_t),
                                  ospm0_state_table);

    fwk_mm_calloc_ExpectAndReturn(1,
                                  sizeof(struct mod_scmi_clock_agent),
                                  agent_table[FAKE_SCMI_AGENT_IDX_OSPM1]);
    fwk_mm_calloc_ExpectAndReturn(SCMI_CLOCK_OSPM1_COUNT,
                                  sizeof(uint8_t),
                                  ospm1_state_table);

    for (unsigned int i = 0; i < FAKE_SCMI_AGENT_IDX_COUNT; i++) {
        status = scmi_clock_element_init(FWK_ID_ELEMENT(FWK_MODULE_IDX_SCMI_CLOCK, i),
                                     0,
                                     element_table[i].data);
        TEST_ASSERT_EQUAL(FWK_SUCCESS, status);
    }

    TEST_ASSERT_EQUAL_PTR(
        ospm0_state_table,
        scmi_clock_ctx.agent_table[FAKE_SCMI_AGENT_IDX_OSPM0]->state_table);

    TEST_ASSERT_EQUAL_PTR(
        ospm1_state_table,
        scmi_clock_ctx.agent_table[FAKE_SCMI_AGENT_IDX_OSPM1]->state_table);

    TEST_ASSERT_EQUAL_UINT8_ARRAY(
        ospm0_state_table_default,
        scmi_clock_ctx.agent_table[FAKE_SCMI_AGENT_IDX_OSPM0]->state_table,
        SCMI_CLOCK_OSPM0_COUNT);

    TEST_ASSERT_EQUAL_UINT8_ARRAY(
        ospm1_state_table_default,
        scmi_clock_ctx.agent_table[FAKE_SCMI_AGENT_IDX_OSPM1]->state_table,
        SCMI_CLOCK_OSPM1_COUNT);

    TEST_ASSERT_EQUAL_UINT8_ARRAY(
        dev_clock_ref_count_table_default,
        dev_clock_ref_count_table,
        CLOCK_DEV_IDX_COUNT);
}

void test_find_agent_scmi_clock_idx(void)
{
    int status;
    unsigned int scmi_clock_idx;

    fwk_id_get_element_idx_Stub(get_element_idx_callback);

    /* agent id is 0, i.e. platform*/
    status = find_agent_scmi_clock_idx(0, CLOCK_DEV_IDX_FAKE3, &scmi_clock_idx);

    TEST_ASSERT_EQUAL(status, FWK_E_DATA);

    /* agent id is FAKE_SCMI_AGENT_IDX_PSCI */
    status = find_agent_scmi_clock_idx(FAKE_SCMI_AGENT_IDX_PSCI, CLOCK_DEV_IDX_FAKE3, &scmi_clock_idx);

    TEST_ASSERT_EQUAL(status, FWK_E_DATA);

    /* agent id is FAKE_SCMI_AGENT_IDX_OSPM0 */
    status = find_agent_scmi_clock_idx(FAKE_SCMI_AGENT_IDX_OSPM0, CLOCK_DEV_IDX_FAKE3, &scmi_clock_idx);

    TEST_ASSERT_EQUAL(status, FWK_SUCCESS);
    TEST_ASSERT_EQUAL(scmi_clock_idx, SCMI_CLOCK_OSPM0_IDX3);

    /* agent id is FAKE_SCMI_AGENT_IDX_OSPM1 */
    status = find_agent_scmi_clock_idx(FAKE_SCMI_AGENT_IDX_OSPM1, CLOCK_DEV_IDX_FAKE3, &scmi_clock_idx);

    TEST_ASSERT_EQUAL(status, FWK_SUCCESS);
    TEST_ASSERT_EQUAL(scmi_clock_idx, SCMI_CLOCK_OSPM1_IDX0);

    /* agent id is FAKE_SCMI_AGENT_IDX_OSPM1 but the clock is not accessible by
    the agent */
    status = find_agent_scmi_clock_idx(FAKE_SCMI_AGENT_IDX_OSPM1, CLOCK_DEV_IDX_FAKE0, &scmi_clock_idx);

    TEST_ASSERT_EQUAL(status, FWK_E_DATA);
}

int scmi_test_main(void)
{
    UNITY_BEGIN();
        RUN_TEST(test_scmi_clock_init);
        RUN_TEST(test_scmi_clock_element_init);
    #if defined(BUILD_HAS_MOD_RESOURCE_PERMS)
        RUN_TEST(test_function_set_rate);
    #else
        RUN_TEST(test_function_set_rate);
        RUN_TEST(test_set_rate_with_invalid_message_id_expect_SCMI_NOT_FOUND);
        RUN_TEST(
            test_set_rate_with_invalid_payload_size_expect_SCMI_PROTOCOL_ERROR);

        RUN_TEST(test_clock_ref_count_allocate);
        RUN_TEST(test_mod_scmi_clock_request_state_check_no_change_running);
        RUN_TEST(test_mod_scmi_clock_request_state_check_no_change_stopped);
        RUN_TEST(test_mod_scmi_clock_request_state_check_ref_count_0_running);
        RUN_TEST(test_mod_scmi_clock_request_state_check_ref_count_1_stopped);
        RUN_TEST(test_mod_scmi_clock_request_state_check_ref_count_0_stopped);
        RUN_TEST(test_mod_scmi_clock_request_state_check_ref_count_1_running);
        RUN_TEST(test_mod_scmi_clock_request_state_check_ref_count_2_stopped);
        RUN_TEST(test_mod_scmi_clock_state_update_no_change_running);
        RUN_TEST(test_mod_scmi_clock_state_update_no_change_stopped);
        RUN_TEST(test_mod_scmi_clock_state_update_ref_count_0_running);
        RUN_TEST(test_mod_scmi_clock_state_update_ref_count_1_running);
        RUN_TEST(test_mod_scmi_clock_state_update_ref_count_2_stopped);
        RUN_TEST(test_mod_scmi_clock_state_update_ref_count_1_stopped);

        RUN_TEST(test_mod_scmi_clock_attributes_handler_invalid_clock_id);
        RUN_TEST(test_mod_scmi_clock_attributes_handler_get_state);

        RUN_TEST(test_mod_scmi_clock_protocol_version_handler);
        RUN_TEST(test_mod_scmi_clock_protocol_attributes_handler);
        RUN_TEST(
            test_protocol_message_attributes_handler_notify_rate);
        RUN_TEST(
            test_protocol_message_attributes_handler_notify_rate_requested);
        RUN_TEST(test_process_request_event_get_state);
        RUN_TEST(test_process_request_event_get_clock_extended_name);
        RUN_TEST(test_scmi_clock_name_get_handler_success);
        RUN_TEST(test_scmi_clock_name_get_handler_extended_name_not_supported);
#ifdef BUILD_HAS_SCMI_NOTIFICATIONS
        RUN_TEST(test_clock_rate_changed_notify_handler_invalid_clock_id);
        RUN_TEST(test_clock_rate_changed_notify_handler_add_subscriber);
        RUN_TEST(test_clock_rate_changed_notify_handler_remove_subscriber);

        RUN_TEST(
            test_clock_rate_change_requested_notify_handler_add_subscriber);
        RUN_TEST(
            test_clock_rate_change_requested_notify_handler_remove_subscriber);

        RUN_TEST(
            test_mod_scmi_clock_process_notification_rate_changed);
        RUN_TEST(
            test_mod_scmi_clock_process_notification_rate_change_requested);

        RUN_TEST(test_mod_scmi_clock_start);
        RUN_TEST(test_mod_scmi_clock_bind);
#endif
        RUN_TEST(test_find_agent_scmi_clock_idx);

    #endif
    return UNITY_END();
}

#if !defined(TEST_ON_TARGET)
int main(void)
{
    return scmi_test_main();
}
#endif
