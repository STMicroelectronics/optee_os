/*
 * Arm SCP/MCP Software
 * Copyright (c) 2024, Arm Limited and Contributors. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include "scp_unity.h"
#include "unity.h"

#include <Mockfwk_core.h>
#include <Mockfwk_id.h>
#include <Mockfwk_module.h>
#include <Mockmod_spmi_extra.h>
#include <internal/Mockfwk_core_internal.h>

#include UNIT_TEST_SRC

enum spmi_element {
    SPMI_INSTANCE_0,
    SPMI_INSTANCE_COUNT,
};

static const struct fwk_element test_spmi_element_table[] = {
    [0] = {
        .name = "SPMI0",
        .data = &(struct mod_spmi_dev_config) {
            .driver_id = FWK_ID_ELEMENT_INIT(FWK_MODULE_IDX_SPMI_CONTROLLER, 0),
            .api_id = FWK_ID_API_INIT(FWK_MODULE_IDX_SPMI_CONTROLLER,
                            MOD_SPMI_API_IDX_SPMI),
        },
    },
    [1] = {0},
};

static const struct fwk_element *spmi_get_element_table(fwk_id_t module_id)
{
    return test_spmi_element_table;
}

struct fwk_module_config test_config_spmi = {
    .elements = FWK_MODULE_DYNAMIC_ELEMENTS(spmi_get_element_table),
};

struct mod_spmi_driver_api *driver_api = &(struct mod_spmi_driver_api){
    .send_command = mod_spmi_driver_send_command,
};

void setUp(void)
{
    /* Set up device context */
    ctx_table = fwk_mm_calloc(SPMI_INSTANCE_COUNT, sizeof(ctx_table[0]));

    ctx_table[0].config =
        (struct mod_spmi_dev_config *)test_spmi_element_table[0].data;
    ctx_table[0].driver_api = driver_api;
}

void tearDown(void)
{
    /* Do nothing */
}

void test_mod_spmi_send_completer_read_command_success(void)
{
    int status;
    static uint8_t buffer[4];

    __fwk_put_event_ExpectAnyArgsAndReturn(FWK_SUCCESS);
    status = mod_spmi_send_completer_read_command(
        fwk_module_id_spmi_controller_element, 0, 0, buffer, 4);
    TEST_ASSERT_EQUAL(FWK_PENDING, status);
}

void test_mod_spmi_send_completer_read_command_fail(void)
{
    int status;
    static uint8_t buffer[4];

    /* Test by passing NULL pointer instead of data */
    status = mod_spmi_send_completer_read_command(
        fwk_module_id_spmi_controller_element, 0, 0, NULL, 4);
    TEST_ASSERT_EQUAL(FWK_E_PARAM, status);

    /* Test by passing 0 byte count */
    status = mod_spmi_send_completer_read_command(
        fwk_module_id_spmi_controller_element, 0, 0, buffer, 0);
    TEST_ASSERT_EQUAL(FWK_E_PARAM, status);

    /* Test with failure in creating spmi request */
    __fwk_put_event_ExpectAnyArgsAndReturn(FWK_E_PARAM);
    status = mod_spmi_send_completer_read_command(
        fwk_module_id_spmi_controller_element, 0, 0, buffer, 4);
    TEST_ASSERT_EQUAL(FWK_E_PARAM, status);
}

void test_mod_spmi_send_completer_write_command_success(void)
{
    int status;
    static uint8_t buffer[4];

    __fwk_put_event_ExpectAnyArgsAndReturn(FWK_SUCCESS);
    status = mod_spmi_send_completer_write_command(
        fwk_module_id_spmi_controller_element, 0, 0, buffer, 4);
    TEST_ASSERT_EQUAL(FWK_PENDING, status);
}

void test_mod_spmi_send_completer_write_command(void)
{
    int status;
    static uint8_t buffer[4];

    /* Test by passing NULL pointer instead of data */
    status = mod_spmi_send_completer_write_command(
        fwk_module_id_spmi_controller_element, 0, 0, NULL, 4);
    TEST_ASSERT_EQUAL(FWK_E_PARAM, status);

    /* Test by passing 0 byte count */
    status = mod_spmi_send_completer_write_command(
        fwk_module_id_spmi_controller_element, 0, 0, buffer, 0);
    TEST_ASSERT_EQUAL(FWK_E_PARAM, status);

    /* Test with failure in creating spmi request */
    __fwk_put_event_ExpectAnyArgsAndReturn(FWK_E_PARAM);
    status = mod_spmi_send_completer_write_command(
        fwk_module_id_spmi_controller_element, 0, 0, buffer, 4);
    TEST_ASSERT_EQUAL(FWK_E_PARAM, status);
}

void test_mod_spmi_send_power_command_success(void)
{
    int status;

    __fwk_put_event_ExpectAnyArgsAndReturn(FWK_SUCCESS);
    status = mod_spmi_send_power_command(
        fwk_module_id_spmi_controller_element, 0, MOD_SPMI_CMD_RESET);
    TEST_ASSERT_EQUAL(FWK_PENDING, status);
}

void test_mod_spmi_send_power_command_fail(void)
{
    int status;

    /* Test with failure in creating spmi request */
    __fwk_put_event_ExpectAnyArgsAndReturn(FWK_E_PARAM);
    status = mod_spmi_send_power_command(
        fwk_module_id_spmi_controller_element, 0, MOD_SPMI_CMD_RESET);
    TEST_ASSERT_EQUAL(FWK_E_PARAM, status);
}

void test_mod_spmi_send_auth_command_success(void)
{
    int status;
    uint8_t buffer[4];

    __fwk_put_event_ExpectAnyArgsAndReturn(FWK_SUCCESS);
    status = mod_spmi_send_auth_command(
        fwk_module_id_spmi_controller_element, 0, buffer, 4);
    TEST_ASSERT_EQUAL(FWK_PENDING, status);
}

void test_mod_spmi_send_requester_read_success(void)
{
    int status;
    uint8_t buffer[4];

    __fwk_put_event_ExpectAnyArgsAndReturn(FWK_SUCCESS);
    status = mod_spmi_send_requester_read(
        fwk_module_id_spmi_controller_element, 0, 0, buffer);
    TEST_ASSERT_EQUAL(FWK_PENDING, status);
}

void test_mod_spmi_send_requester_write_success(void)
{
    int status;
    uint8_t buffer[4];

    __fwk_put_event_ExpectAnyArgsAndReturn(FWK_SUCCESS);
    status = mod_spmi_send_requester_write(
        fwk_module_id_spmi_controller_element, 0, 0, buffer);
    TEST_ASSERT_EQUAL(FWK_PENDING, status);
}

void test_mod_spmi_send_ddb_read_success(void)
{
    int status;
    uint8_t buffer[4];

    __fwk_put_event_ExpectAnyArgsAndReturn(FWK_SUCCESS);
    status = mod_spmi_send_ddb_read(
        fwk_module_id_spmi_controller_element, 0, 1, buffer);
    TEST_ASSERT_EQUAL(FWK_PENDING, status);
}

void test_mod_spmi_init(void)
{
    int status;

    status = mod_spmi_init(fwk_module_id_spmi, 1, NULL);
    TEST_ASSERT_EQUAL(FWK_SUCCESS, status);
}

void test_create_spmi_request_success(void)
{
    int status;
    struct mod_spmi_request request;

    request.command = MOD_SPMI_CMD_RESET;
    __fwk_put_event_ExpectAnyArgsAndReturn(FWK_SUCCESS);
    status =
        create_spmi_request(fwk_module_id_spmi_controller_element, &request);
    TEST_ASSERT_EQUAL(FWK_PENDING, status);
}

void test_create_spmi_request_fail(void)
{
    int status;
    struct mod_spmi_request request;

    request.command = MOD_SPMI_CMD_RESET;
    __fwk_put_event_ExpectAnyArgsAndReturn(FWK_E_PARAM);
    status =
        create_spmi_request(fwk_module_id_spmi_controller_element, &request);
    TEST_ASSERT_EQUAL(FWK_E_PARAM, status);
}

void test_mod_spmi_device_init(void)
{
    int status;
    void *data = (void *)&test_spmi_element_table[0].data;

    fwk_module_is_valid_element_id_ExpectAndReturn(
        fwk_module_id_spmi_controller_element, true);
    fwk_id_get_element_idx_ExpectAndReturn(
        fwk_module_id_spmi_controller_element, 0);
    status = mod_spmi_dev_init(fwk_module_id_spmi_controller_element, 0, data);
    TEST_ASSERT_EQUAL(FWK_SUCCESS, status);
}

void test_respond_to_caller_success(void)
{
    int status;

    fwk_get_first_delayed_response_ExpectAnyArgsAndReturn(FWK_SUCCESS);
    __fwk_put_event_ExpectAnyArgsAndReturn(FWK_SUCCESS);
    status =
        respond_to_caller(fwk_module_id_spmi_controller_element, FWK_SUCCESS);
    TEST_ASSERT_EQUAL(FWK_SUCCESS, status);
}

void test_respond_to_caller_fail(void)
{
    int status;

    fwk_get_first_delayed_response_ExpectAnyArgsAndReturn(FWK_E_PARAM);
    status =
        respond_to_caller(fwk_module_id_spmi_controller_element, FWK_SUCCESS);
    TEST_ASSERT_EQUAL(FWK_E_PARAM, status);

    fwk_get_first_delayed_response_ExpectAnyArgsAndReturn(FWK_SUCCESS);
    __fwk_put_event_ExpectAnyArgsAndReturn(FWK_E_PARAM);
    status =
        respond_to_caller(fwk_module_id_spmi_controller_element, FWK_SUCCESS);
    TEST_ASSERT_EQUAL(FWK_E_PARAM, status);
}

void test_transaction_completed(void)
{
    __fwk_put_event_ExpectAnyArgsAndReturn(FWK_SUCCESS);
    transaction_completed(fwk_module_id_spmi_controller_element, FWK_SUCCESS);
}

void test_mod_spmi_bind(void)
{
    int status;

    /* Bind for round > 0 should return FWK_SUCCESS */
    status = mod_spmi_bind(fwk_module_id_spmi_controller_element, 1);
    TEST_ASSERT_EQUAL(FWK_SUCCESS, status);

    /* Bind for fwk_id_t module should return FWK_SUCCESS */
    fwk_id_is_type_ExpectAndReturn(
        fwk_module_id_spmi_controller, FWK_ID_TYPE_MODULE, true);
    status = mod_spmi_bind(fwk_module_id_spmi_controller, 0);
    TEST_ASSERT_EQUAL(FWK_SUCCESS, status);

    fwk_module_bind_ExpectAndReturn(
        ctx_table[0].config->driver_id,
        ctx_table[0].config->api_id,
        &ctx_table[0].driver_api,
        FWK_SUCCESS);
    fwk_id_is_type_ExpectAndReturn(
        fwk_module_id_spmi_controller_element, FWK_ID_TYPE_MODULE, false);
    fwk_module_is_valid_element_id_ExpectAndReturn(
        fwk_module_id_spmi_controller_element, true);
    fwk_id_get_element_idx_ExpectAndReturn(
        fwk_module_id_spmi_controller_element, 0);
    status = mod_spmi_bind(fwk_module_id_spmi_controller_element, 0);
    TEST_ASSERT_EQUAL(FWK_SUCCESS, status);
}

void test_mod_spmi_process_bind_request_success(void)
{
    int status;
    static struct mod_spmi_driver_response_api *spmi_driver_response_api;

    fwk_id_is_type_ExpectAndReturn(
        fwk_module_id_spmi_element, FWK_ID_TYPE_ELEMENT, true);
    fwk_module_is_valid_element_id_ExpectAndReturn(
        fwk_module_id_spmi_element, true);
    fwk_id_get_element_idx_ExpectAndReturn(fwk_module_id_spmi_element, 0);
    fwk_id_is_equal_ExpectAndReturn(
        fwk_module_id_spmi_controller_element,
        fwk_module_id_spmi_controller_element,
        true);
    fwk_id_is_equal_ExpectAndReturn(
        mod_spmi_api_id_driver_response, mod_spmi_api_id_driver_response, true);
    fwk_id_get_api_idx_ExpectAndReturn(
        mod_spmi_api_id_driver_response, MOD_SPMI_API_IDX_DRIVER_RESPONSE);
    status = mod_spmi_process_bind_request(
        fwk_module_id_spmi_controller_element,
        fwk_module_id_spmi_element,
        mod_spmi_api_id_driver_response,
        (const void **)&spmi_driver_response_api);
    TEST_ASSERT_EQUAL(FWK_SUCCESS, status);
    TEST_ASSERT_EQUAL_PTR(&driver_response_api, spmi_driver_response_api);
}

void test_mod_spmi_process_bind_request_fail(void)
{
    int status;

    fwk_id_is_type_ExpectAndReturn(
        fwk_module_id_spmi_controller, FWK_ID_TYPE_ELEMENT, false);
    status = mod_spmi_process_bind_request(
        fwk_module_id_spmi_controller,
        fwk_module_id_spmi_controller,
        fwk_module_id_spmi_controller,
        NULL);
    TEST_ASSERT_EQUAL(FWK_E_PARAM, status);
}

void test_process_next_request_success(void)
{
    int status;
    struct mod_spmi_dev_ctx *ctx = &ctx_table[0];
    bool empty;
    struct fwk_event delayed_response;

    empty = true;
    fwk_is_delayed_response_list_empty_ExpectAnyArgsAndReturn(FWK_SUCCESS);
    fwk_is_delayed_response_list_empty_ReturnThruPtr_is_empty(&empty);
    status = process_next_request(fwk_module_id_spmi_controller_element, ctx);
    TEST_ASSERT_EQUAL(FWK_SUCCESS, status);
    TEST_ASSERT_EQUAL(ctx->state, MOD_SPMI_DEV_STATE_IDLE);

    empty = false;
    fwk_is_delayed_response_list_empty_ExpectAnyArgsAndReturn(FWK_SUCCESS);
    fwk_is_delayed_response_list_empty_ReturnThruPtr_is_empty(&empty);
    fwk_is_delayed_response_list_empty_ExpectAnyArgsAndReturn(FWK_SUCCESS);
    fwk_get_first_delayed_response_ExpectAnyArgsAndReturn(FWK_SUCCESS);
    fwk_get_first_delayed_response_ReturnThruPtr_event(&delayed_response);
    fwk_get_first_delayed_response_ExpectAnyArgsAndReturn(FWK_SUCCESS);
    fwk_id_get_event_idx_ExpectAnyArgsAndReturn(
        MOD_SPMI_EVENT_IDX_SEND_COMMAND);
    mod_spmi_driver_send_command_ExpectAnyArgsAndReturn(FWK_PENDING);
    status = process_next_request(fwk_module_id_spmi_controller_element, ctx);
    TEST_ASSERT_EQUAL(FWK_SUCCESS, status);
}

void test_process_next_request_fail(void)
{
    int status;
    struct mod_spmi_dev_ctx *ctx = &ctx_table[0];
    bool empty = true;
    struct fwk_event delayed_response;

    fwk_is_delayed_response_list_empty_ExpectAnyArgsAndReturn(FWK_E_PARAM);
    mod_spmi_driver_send_command_ExpectAnyArgsAndReturn(FWK_E_PARAM);
    __fwk_put_event_ExpectAnyArgsAndReturn(FWK_E_PARAM);
    __fwk_put_event_light_ExpectAnyArgsAndReturn(FWK_E_PARAM);
    status = process_next_request(fwk_module_id_spmi_controller_element, ctx);
    TEST_ASSERT_EQUAL(FWK_E_PARAM, status);

    empty = false;
    fwk_is_delayed_response_list_empty_ExpectAnyArgsAndReturn(FWK_SUCCESS);
    fwk_is_delayed_response_list_empty_ReturnThruPtr_is_empty(&empty);
    fwk_is_delayed_response_list_empty_ExpectAnyArgsAndReturn(FWK_SUCCESS);
    __fwk_put_event_light_ExpectAnyArgsAndReturn(FWK_E_PARAM);
    fwk_get_first_delayed_response_ExpectAnyArgsAndReturn(FWK_E_PARAM);
    status = process_next_request(fwk_module_id_spmi_controller_element, ctx);
    TEST_ASSERT_EQUAL(FWK_E_PARAM, status);

    empty = false;
    fwk_is_delayed_response_list_empty_ExpectAnyArgsAndReturn(FWK_SUCCESS);
    fwk_is_delayed_response_list_empty_ReturnThruPtr_is_empty(&empty);
    fwk_get_first_delayed_response_ExpectAnyArgsAndReturn(FWK_SUCCESS);
    fwk_get_first_delayed_response_ReturnThruPtr_event(&delayed_response);
    fwk_id_get_event_idx_ExpectAnyArgsAndReturn(
        MOD_SPMI_EVENT_IDX_SEND_COMMAND);
    mod_spmi_driver_send_command_ExpectAnyArgsAndReturn(FWK_E_PARAM);
    __fwk_put_event_ExpectAnyArgsAndReturn(FWK_E_PARAM);
    status = process_next_request(fwk_module_id_spmi_controller_element, ctx);
    TEST_ASSERT_EQUAL(FWK_E_PARAM, status);
}

void test_mod_spmi_process_send_command_success(void)
{
    int status;
    struct fwk_event event, resp_event;
    struct mod_spmi_request request;
    struct mod_spmi_request *event_param =
        (struct mod_spmi_request *)event.params;

    request.command = MOD_SPMI_CMD_EXT_REG_READ_LONG;

    event.source_id = fwk_module_id_spmi_controller_element;
    event.target_id = fwk_module_id_spmi_controller_element;
    event.is_response = false;
    event.response_requested = false;
    event.is_notification = false;
    event.is_delayed_response = false;
    event.id = mod_spmi_event_id_send_command;
    *event_param = request;

    fwk_module_is_valid_element_id_ExpectAndReturn(
        fwk_module_id_spmi_controller_element, true);
    fwk_id_get_event_idx_ExpectAnyArgsAndReturn(
        MOD_SPMI_EVENT_IDX_SEND_COMMAND);
    fwk_id_get_event_idx_ExpectAnyArgsAndReturn(
        MOD_SPMI_EVENT_IDX_SEND_COMMAND);
    fwk_id_get_element_idx_ExpectAndReturn(
        fwk_module_id_spmi_controller_element, 0);
    mod_spmi_driver_send_command_ExpectAnyArgsAndReturn(FWK_SUCCESS);
    status = mod_spmi_process_send_command(&ctx_table[0], &event, &resp_event);
    TEST_ASSERT_EQUAL(FWK_SUCCESS, status);
}

void test_mod_spmi_process_event_success(void)
{
    int status;
    struct fwk_event event, resp_event;
    struct mod_spmi_request request;
    struct mod_spmi_request *event_param =
        (struct mod_spmi_request *)event.params;

    request.command = MOD_SPMI_CMD_EXT_REG_READ_LONG;

    event.source_id = fwk_module_id_spmi_controller_element;
    event.target_id = fwk_module_id_spmi_controller_element;
    event.is_response = false;
    event.response_requested = false;
    event.is_notification = false;
    event.is_delayed_response = false;
    event.id = mod_spmi_event_id_send_command;
    *event_param = request;

    fwk_module_is_valid_element_id_ExpectAndReturn(
        fwk_module_id_spmi_controller_element, true);
    fwk_id_get_event_idx_ExpectAnyArgsAndReturn(
        MOD_SPMI_EVENT_IDX_SEND_COMMAND);
    fwk_id_get_event_idx_ExpectAnyArgsAndReturn(
        MOD_SPMI_EVENT_IDX_SEND_COMMAND);
    fwk_id_get_element_idx_ExpectAndReturn(
        fwk_module_id_spmi_controller_element, 0);
    mod_spmi_driver_send_command_ExpectAnyArgsAndReturn(FWK_SUCCESS);
    status = mod_spmi_process_send_command(&ctx_table[0], &event, &resp_event);
    TEST_ASSERT_EQUAL(FWK_SUCCESS, status);
}

int spmi_test_main(void)
{
    UNITY_BEGIN();

    RUN_TEST(test_mod_spmi_init);
    RUN_TEST(test_mod_spmi_device_init);
    RUN_TEST(test_mod_spmi_bind);
    RUN_TEST(test_mod_spmi_process_bind_request_success);
    RUN_TEST(test_mod_spmi_process_bind_request_fail);
    RUN_TEST(test_respond_to_caller_success);
    RUN_TEST(test_respond_to_caller_fail);
    RUN_TEST(test_transaction_completed);
    RUN_TEST(test_process_next_request_success);
    RUN_TEST(test_process_next_request_fail);
    RUN_TEST(test_mod_spmi_process_event_success);
    RUN_TEST(test_mod_spmi_process_send_command_success);
    RUN_TEST(test_create_spmi_request_success);
    RUN_TEST(test_create_spmi_request_fail);
    RUN_TEST(test_mod_spmi_send_completer_read_command_success);
    RUN_TEST(test_mod_spmi_send_completer_read_command_fail);
    RUN_TEST(test_mod_spmi_send_completer_write_command_success);
    RUN_TEST(test_mod_spmi_send_power_command_fail);
    RUN_TEST(test_mod_spmi_send_auth_command_success);
    RUN_TEST(test_mod_spmi_send_requester_read_success);
    RUN_TEST(test_mod_spmi_send_requester_write_success);
    RUN_TEST(test_mod_spmi_send_ddb_read_success);

    return UNITY_END();
}

#if !defined(TEST_ON_TARGET)
int main(void)
{
    return spmi_test_main();
}
#endif
