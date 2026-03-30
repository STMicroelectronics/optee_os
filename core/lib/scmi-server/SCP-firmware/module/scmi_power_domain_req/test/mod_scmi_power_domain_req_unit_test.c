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
#include <Mockfwk_notification.h>
#include <Mockfwk_string.h>
#include <Mockmod_scmi_power_domain_req_extra.h>
#include <internal/Mockfwk_core_internal.h>

#include <mod_scmi.h>

#include <fwk_element.h>
#include <fwk_macros.h>

#include UNIT_TEST_SRC

enum scp_pow_domain_req_nums {
    MOD_SCMI_POWER_DOMAIN_REQ_IDX_0,
    MOD_SCMI_POWER_DOMAIN_REQ_IDX_1,
    MOD_SCMI_POWER_DOMAIN_REQ_COUNT,
};

/*
 * Power Domain Req module config
 */
static const struct fwk_element
    power_domain_req_element_table[MOD_SCMI_POWER_DOMAIN_REQ_COUNT + 1] = {
    [MOD_SCMI_POWER_DOMAIN_REQ_IDX_0] = {
        .name = "Fake Power Domain Req",
        .data = &((struct scmi_power_domain_req_dev_config){
            .start_state = 1,
            .domain_id = 0,
            .service_id = FWK_ID_ELEMENT_INIT(
                FWK_MODULE_IDX_SCMI,
                0),
            .timer_id = FWK_ID_SUB_ELEMENT_INIT(
                FWK_MODULE_IDX_TIMER,
                0,
                0),
            .response_timeout = 30000,
            .pd_element_id = FWK_ID_ELEMENT_INIT(
                FWK_MODULE_IDX_POWER_DOMAIN,
                0),
            .sync_flag = true,
        }),
    },
    [MOD_SCMI_POWER_DOMAIN_REQ_IDX_1] = {
        .name = "Fake Power Domain Req",
        .data = &((struct scmi_power_domain_req_dev_config){
            .start_state = 1,
            .domain_id = 1,
            .service_id = FWK_ID_ELEMENT_INIT(
                FWK_MODULE_IDX_SCMI,
                0),
            .timer_id = FWK_ID_SUB_ELEMENT_INIT(
                FWK_MODULE_IDX_TIMER,
                0,
                0),
            .response_timeout = 30000,
            .pd_element_id = FWK_ID_ELEMENT_INIT(
                FWK_MODULE_IDX_POWER_DOMAIN,
                0),
            .sync_flag = false,
        }),
    },
    [MOD_SCMI_POWER_DOMAIN_REQ_COUNT] = { 0 },
};

static const struct fwk_element *get_power_domain_req_element_table(
    fwk_id_t module_id)
{
    return power_domain_req_element_table;
}

const struct fwk_module_config config_scmi_power_domain_req = {
    .elements = FWK_MODULE_DYNAMIC_ELEMENTS(get_power_domain_req_element_table),
};

const struct mod_scmi_from_protocol_req_api scmi_api = {
    .scmi_send_message = scmi_send_message,
    .response_message_handler = response_message_handler,
};

static struct mod_timer_alarm_api alarm_api = {
    .start = start_alarm_api,
    .stop = stop_alarm_api,
};

const struct mod_pd_driver_input_api pd_driver_input_api = {
    .report_power_state_transition = report_power_state_transition,
};

static struct scmi_power_domain_req_dev_ctx
    dev_ctx[MOD_SCMI_POWER_DOMAIN_REQ_COUNT];

void setUp(void)
{
    memset(&mod_ctx, 0, sizeof(mod_ctx));
    memset(
        &dev_ctx[MOD_SCMI_POWER_DOMAIN_REQ_IDX_0],
        0,
        sizeof(struct scmi_power_domain_req_dev_ctx));
    dev_ctx[MOD_SCMI_POWER_DOMAIN_REQ_IDX_0].config =
        (const struct scmi_power_domain_req_dev_config *)
            power_domain_req_element_table[MOD_SCMI_POWER_DOMAIN_REQ_IDX_0]
                .data;

    memset(
        &dev_ctx[MOD_SCMI_POWER_DOMAIN_REQ_IDX_1],
        0,
        sizeof(struct scmi_power_domain_req_dev_ctx));
    dev_ctx[MOD_SCMI_POWER_DOMAIN_REQ_IDX_1].config =
        (const struct scmi_power_domain_req_dev_config *)
            power_domain_req_element_table[MOD_SCMI_POWER_DOMAIN_REQ_IDX_1]
                .data;

    mod_ctx.dev_ctx_table = dev_ctx;
    mod_ctx.dev_count = MOD_SCMI_POWER_DOMAIN_REQ_COUNT;
    mod_ctx.scmi_api = &scmi_api;
    mod_ctx.alarm_api = &alarm_api;
    mod_ctx.token = 0;
    mod_ctx.pd_driver_input_api = &pd_driver_input_api;

    handler_table[MOD_SCMI_POWER_DOMAIN_REQ_STATE_SET] = fake_message_handler;
}

void tearDown(void)
{
}

void test_function_scmi_power_domain_req_init_error(void)
{
    int status;

    /* Clear module context to ensure it is properly initialized */
    memset(&mod_ctx, 0, sizeof(mod_ctx));

    status = scmi_power_domain_req_init(
        fwk_module_id_scmi_power_domain_req, 0, NULL);
    TEST_ASSERT_EQUAL(status, FWK_E_SUPPORT);
}

void test_function_scmi_power_domain_req_init_success(void)
{
    int status;

    fwk_mm_calloc_ExpectAndReturn(
        MOD_SCMI_POWER_DOMAIN_REQ_COUNT,
        sizeof(struct scmi_power_domain_req_dev_ctx),
        (void *)dev_ctx);

    status = scmi_power_domain_req_init(
        fwk_module_id_scmi_power_domain_req,
        MOD_SCMI_POWER_DOMAIN_REQ_COUNT,
        NULL);
    TEST_ASSERT_EQUAL(status, FWK_SUCCESS);
    TEST_ASSERT_EQUAL(mod_ctx.dev_count, MOD_SCMI_POWER_DOMAIN_REQ_COUNT);
    TEST_ASSERT_EQUAL(mod_ctx.dev_ctx_table, dev_ctx);
}

void test_function_scmi_power_domain_req_elem_init_0(void)
{
    int status;
    fwk_id_t element_id = FWK_ID_ELEMENT(
        FWK_MODULE_IDX_SCMI_POWER_DOMAIN_REQ, MOD_SCMI_POWER_DOMAIN_REQ_IDX_0);

    fwk_id_get_element_idx_ExpectAndReturn(
        element_id, MOD_SCMI_POWER_DOMAIN_REQ_IDX_0);
    fwk_module_is_valid_sub_element_id_ExpectAnyArgsAndReturn(true);

    status = scmi_power_domain_req_elem_init(
        element_id,
        0,
        power_domain_req_element_table[MOD_SCMI_POWER_DOMAIN_REQ_IDX_0].data);
    TEST_ASSERT_EQUAL(status, FWK_SUCCESS);
    TEST_ASSERT_EQUAL(
        dev_ctx[MOD_SCMI_POWER_DOMAIN_REQ_IDX_0].current_state,
        ((struct scmi_power_domain_req_dev_config *)
             power_domain_req_element_table[MOD_SCMI_POWER_DOMAIN_REQ_IDX_0]
                 .data)
            ->start_state);
}

void test_function_scmi_power_domain_req_elem_init_0_error(void)
{
    int status;
    fwk_id_t element_id = FWK_ID_ELEMENT(
        FWK_MODULE_IDX_SCMI_POWER_DOMAIN_REQ, MOD_SCMI_POWER_DOMAIN_REQ_IDX_0);

    fwk_id_get_element_idx_ExpectAndReturn(
        element_id, MOD_SCMI_POWER_DOMAIN_REQ_IDX_0);
    fwk_module_is_valid_sub_element_id_ExpectAnyArgsAndReturn(false);

    status = scmi_power_domain_req_elem_init(
        element_id,
        0,
        power_domain_req_element_table[MOD_SCMI_POWER_DOMAIN_REQ_IDX_0].data);
    TEST_ASSERT_EQUAL(status, FWK_E_SUPPORT);
}

void test_function_scmi_power_domain_req_elem_init_0_null_data(void)
{
    int status;
    fwk_id_t element_id = FWK_ID_ELEMENT(
        FWK_MODULE_IDX_SCMI_POWER_DOMAIN_REQ, MOD_SCMI_POWER_DOMAIN_REQ_IDX_0);

    fwk_id_get_element_idx_ExpectAndReturn(
        element_id, MOD_SCMI_POWER_DOMAIN_REQ_IDX_0);

    status = scmi_power_domain_req_elem_init(element_id, 0, NULL);
    TEST_ASSERT_EQUAL(status, FWK_E_PANIC);
}

void test_function_scmi_power_domain_req_elem_init_1(void)
{
    int status;
    fwk_id_t element_id = FWK_ID_ELEMENT(
        FWK_MODULE_IDX_SCMI_POWER_DOMAIN_REQ, MOD_SCMI_POWER_DOMAIN_REQ_IDX_1);

    /* Clear element context to ensure it is properly initialized */
    memset(
        &dev_ctx[MOD_SCMI_POWER_DOMAIN_REQ_IDX_1],
        0,
        sizeof(struct scmi_power_domain_req_dev_ctx));

    fwk_id_get_element_idx_ExpectAndReturn(
        element_id, MOD_SCMI_POWER_DOMAIN_REQ_IDX_1);
    fwk_module_is_valid_sub_element_id_ExpectAnyArgsAndReturn(true);

    status = scmi_power_domain_req_elem_init(
        element_id,
        0,
        power_domain_req_element_table[MOD_SCMI_POWER_DOMAIN_REQ_IDX_1].data);
    TEST_ASSERT_EQUAL(status, FWK_SUCCESS);
    TEST_ASSERT_EQUAL(
        dev_ctx[MOD_SCMI_POWER_DOMAIN_REQ_IDX_1].current_state,
        ((struct scmi_power_domain_req_dev_config *)
             power_domain_req_element_table[MOD_SCMI_POWER_DOMAIN_REQ_IDX_1]
                 .data)
            ->start_state);
}

void test_function_scmi_power_domain_req_elem_init_1_null_data(void)
{
    int status;
    fwk_id_t element_id = FWK_ID_ELEMENT(
        FWK_MODULE_IDX_SCMI_POWER_DOMAIN_REQ, MOD_SCMI_POWER_DOMAIN_REQ_IDX_1);

    fwk_id_get_element_idx_ExpectAndReturn(
        element_id, MOD_SCMI_POWER_DOMAIN_REQ_IDX_1);

    status = scmi_power_domain_req_elem_init(element_id, 0, NULL);
    TEST_ASSERT_EQUAL(status, FWK_E_PANIC);
}

void test_function_scmi_power_domain_req_bind(void)
{
    int status;

    fwk_id_is_type_ExpectAnyArgsAndReturn(true);
    fwk_id_is_type_ExpectAnyArgsAndReturn(true);

    fwk_module_bind_ExpectAndReturn(
        FWK_ID_MODULE(FWK_MODULE_IDX_SCMI),
        FWK_ID_API(FWK_MODULE_IDX_SCMI, MOD_SCMI_API_IDX_PROTOCOL_REQ),
        &mod_ctx.scmi_api,
        FWK_SUCCESS);

    status = scmi_power_domain_req_bind(fwk_module_id_scmi_power_domain_req, 0);
    TEST_ASSERT_EQUAL(status, FWK_SUCCESS);
}

void test_function_scmi_power_domain_req_bind_mod_round_0_success(void)
{
    int status;

    fwk_id_t element_id = FWK_ID_ELEMENT_INIT(
        FWK_MODULE_IDX_SCMI_POWER_DOMAIN_REQ, MOD_SCMI_POWER_DOMAIN_REQ_IDX_0);

    fwk_id_is_type_ExpectAnyArgsAndReturn(false);
    fwk_id_is_type_ExpectAnyArgsAndReturn(true);
    status = scmi_power_domain_req_bind(element_id, 0);
    TEST_ASSERT_EQUAL(status, FWK_SUCCESS);
}

void test_function_scmi_power_domain_req_bind_mod_round_1_success(void)
{
    int status;

    fwk_id_is_type_ExpectAnyArgsAndReturn(true);
    status = scmi_power_domain_req_bind(fwk_module_id_scmi_power_domain_req, 1);
    TEST_ASSERT_EQUAL(status, FWK_SUCCESS);
}

void test_function_scmi_power_domain_req_bind_element_round_1_success(void)
{
    int status;

    fwk_id_t element_id = FWK_ID_ELEMENT_INIT(
        FWK_MODULE_IDX_SCMI_POWER_DOMAIN_REQ, MOD_SCMI_POWER_DOMAIN_REQ_IDX_0);

    fwk_id_is_type_ExpectAnyArgsAndReturn(false);

    fwk_id_get_element_idx_ExpectAndReturn(
        element_id, MOD_SCMI_POWER_DOMAIN_REQ_IDX_0);

    fwk_module_bind_ExpectAndReturn(
        dev_ctx[MOD_SCMI_POWER_DOMAIN_REQ_IDX_0].config->pd_element_id,
        FWK_ID_API(FWK_MODULE_IDX_POWER_DOMAIN, MOD_PD_API_IDX_DRIVER_INPUT),
        &mod_ctx.pd_driver_input_api,
        FWK_SUCCESS);

    fwk_module_bind_ExpectAndReturn(
        dev_ctx[MOD_SCMI_POWER_DOMAIN_REQ_IDX_0].config->timer_id,
        FWK_ID_API(FWK_MODULE_IDX_TIMER, MOD_TIMER_API_IDX_TIMER),
        &mod_ctx.alarm_api,
        FWK_SUCCESS);

    status = scmi_power_domain_req_bind(element_id, 1);
    TEST_ASSERT_EQUAL(status, FWK_SUCCESS);
}

void test_function_scmi_power_domain_req_process_bind_invalid_api(void)
{
    int status;
    fwk_id_t invalid_id = { .value = UINT32_MAX };
    fwk_id_t target_id;
    struct mod_scmi_to_protocol_api *scmi_api;

    fwk_id_get_api_idx_ExpectAnyArgsAndReturn(MOD_POW_DOMAIN_REQ_API_IDX_COUNT);

    status = scmi_power_domain_req_bind_request(
        invalid_id, target_id, invalid_id, (const void **)&scmi_api);
    TEST_ASSERT_EQUAL(status, FWK_E_SUPPORT);
}

void test_function_scmi_power_domain_req_process_bind_scmi_api_success(void)
{
    int status;
    fwk_id_t target_id;
    struct mod_scmi_to_protocol_api *scmi_api;

    fwk_id_get_api_idx_ExpectAndReturn(
        mod_power_domain_req_scmi_api_id, MOD_POW_DOMAIN_REQ_API_IDX_SCMI_REQ);
    fwk_id_is_equal_ExpectAnyArgsAndReturn(true);

    status = scmi_power_domain_req_bind_request(
        fwk_module_id_scmi,
        target_id,
        mod_power_domain_req_scmi_api_id,
        (const void **)&scmi_api);
    TEST_ASSERT_EQUAL(status, FWK_SUCCESS);
    TEST_ASSERT_NOT_NULL(scmi_api);
}

void test_function_scmi_power_domain_req_process_bind_scmi_api_fail(void)
{
    int status;
    fwk_id_t target_id;
    struct mod_scmi_to_protocol_api *scmi_api;

    fwk_id_get_api_idx_ExpectAndReturn(
        mod_power_domain_req_scmi_api_id, MOD_POW_DOMAIN_REQ_API_IDX_SCMI_REQ);
    fwk_id_is_equal_ExpectAnyArgsAndReturn(false);

    status = scmi_power_domain_req_bind_request(
        fwk_module_id_scmi,
        target_id,
        mod_power_domain_req_scmi_api_id,
        (const void **)&scmi_api);
    TEST_ASSERT_EQUAL(status, FWK_E_ACCESS);
}

void test_function_scmi_power_domain_req_process_bind_PD_api(void)
{
    int status;
    fwk_id_t target_id;
    struct mod_pd_driver_api *pd_driver;

    fwk_id_get_api_idx_ExpectAndReturn(
        mod_power_domain_req_api_id, MOD_POW_DOMAIN_REQ_API_IDX_REQ);

    status = scmi_power_domain_req_bind_request(
        fwk_module_id_power_domain,
        target_id,
        mod_power_domain_req_api_id,
        (const void **)&pd_driver);
    TEST_ASSERT_EQUAL(status, FWK_SUCCESS);
    TEST_ASSERT_NOT_NULL(pd_driver);
}

void test_scmi_power_domain_req_get_scmi_protocol_id(void)
{
    int status;
    fwk_id_t protocol_id;
    uint8_t scmi_protocol_id;
    status = scmi_power_domain_req_get_scmi_protocol_id(
        protocol_id, &scmi_protocol_id);
    TEST_ASSERT_EQUAL(status, FWK_SUCCESS);
    TEST_ASSERT_EQUAL(
        scmi_protocol_id, (uint8_t)MOD_SCMI_PROTOCOL_ID_POWER_DOMAIN);
}

void test_scmi_power_domain_req_message_handler_range(void)
{
    int status;
    fwk_id_t expected_service_id;
    fwk_id_t protocol_id;
    uint8_t payload[sizeof(struct scmi_pd_req_power_state_set_p2a)] = { 0 };

    expected_service_id =
        dev_ctx[MOD_SCMI_POWER_DOMAIN_REQ_IDX_0].config->service_id;

    status = scmi_power_domain_req_message_handler(
        protocol_id,
        expected_service_id,
        (uint32_t *)payload,
        sizeof(payload),
        sizeof(handler_table));
    TEST_ASSERT_EQUAL(status, FWK_E_RANGE);
}

void test_scmi_power_domain_req_message_handler_payload(void)
{
    int status;
    fwk_id_t expected_service_id;
    fwk_id_t protocol_id;
    uint8_t payload[sizeof(struct scmi_pd_req_power_state_set_p2a)] = { 0 };

    expected_service_id =
        dev_ctx[MOD_SCMI_POWER_DOMAIN_REQ_IDX_0].config->service_id;

    status = scmi_power_domain_req_message_handler(
        protocol_id,
        expected_service_id,
        (uint32_t *)payload,
        sizeof(payload) + 1,
        (uint8_t)MOD_SCMI_POWER_DOMAIN_REQ_STATE_SET);
    TEST_ASSERT_EQUAL(status, FWK_E_PARAM);
}

void test_scmi_power_domain_req_message_handler_success(void)
{
    int status;
    fwk_id_t expected_service_id;
    fwk_id_t protocol_id;
    uint8_t payload[sizeof(struct scmi_pd_req_power_state_set_p2a)] = { 0 };

    expected_service_id =
        dev_ctx[MOD_SCMI_POWER_DOMAIN_REQ_IDX_0].config->service_id;

    fake_message_handler_ExpectWithArrayAndReturn(
        expected_service_id,
        (uint32_t *)payload,
        sizeof(payload),
        sizeof(payload),
        FWK_SUCCESS);
    response_message_handler_ExpectAndReturn(expected_service_id, FWK_SUCCESS);

    status = scmi_power_domain_req_message_handler(
        protocol_id,
        expected_service_id,
        (uint32_t *)payload,
        sizeof(payload),
        (uint8_t)MOD_SCMI_POWER_DOMAIN_REQ_STATE_SET);
    TEST_ASSERT_EQUAL(status, FWK_SUCCESS);
}

void test_scmi_power_domain_req_message_handler_param(void)
{
    int status;

    handler_table[MOD_SCMI_POWER_DOMAIN_REQ_STATE_SET] = NULL;

    fwk_id_t expected_service_id;
    fwk_id_t protocol_id;
    uint8_t payload[sizeof(struct scmi_pd_req_power_state_set_p2a)] = { 0 };

    expected_service_id =
        dev_ctx[MOD_SCMI_POWER_DOMAIN_REQ_IDX_0].config->service_id;

    status = scmi_power_domain_req_message_handler(
        protocol_id,
        expected_service_id,
        (uint32_t *)payload,
        sizeof(payload),
        (uint8_t)MOD_SCMI_POWER_DOMAIN_REQ_STATE_SET);
    TEST_ASSERT_EQUAL(status, FWK_E_PARAM);
}

void test_scmi_power_domain_req_get_state(void)
{
    int status;
    fwk_id_t pd_id;
    unsigned int state;

    fwk_id_is_type_ExpectAnyArgsAndReturn(true);
    fwk_id_get_element_idx_ExpectAnyArgsAndReturn(2);

    status = scmi_pd_req_get_state(pd_id, &state);
    TEST_ASSERT_EQUAL(status, FWK_E_DATA);
}

void test_scmi_power_domain_req_get_state_success(void)
{
    int status;
    fwk_id_t pd_id;
    unsigned int state;

    dev_ctx[MOD_SCMI_POWER_DOMAIN_REQ_IDX_1].current_state = 1;
    fwk_id_is_type_ExpectAnyArgsAndReturn(true);
    fwk_id_get_element_idx_ExpectAnyArgsAndReturn(1);

    status = scmi_pd_req_get_state(pd_id, &state);
    TEST_ASSERT_EQUAL(status, FWK_SUCCESS);
    TEST_ASSERT_EQUAL(state, 1);
}

void test_scmi_power_domain_req_get_state_fail(void)
{
    int status;
    fwk_id_t pd_id;
    unsigned int state;

    fwk_id_is_type_ExpectAnyArgsAndReturn(false);

    status = scmi_pd_req_get_state(pd_id, &state);
    TEST_ASSERT_EQUAL(status, FWK_E_DATA);
}

void test_scmi_power_domain_req_set_state_busy(void)
{
    int status;
    fwk_id_t pd_id;
    unsigned int state = 1;

    dev_ctx[MOD_SCMI_POWER_DOMAIN_REQ_IDX_1].is_free = false;
    fwk_id_is_type_ExpectAnyArgsAndReturn(true);
    fwk_id_get_element_idx_ExpectAnyArgsAndReturn(1);

    status = scmi_pd_req_set_state(pd_id, state);
    TEST_ASSERT_EQUAL(status, FWK_E_BUSY);
}

void test_scmi_power_domain_req_set_state_incorrect_id(void)
{
    int status;
    fwk_id_t pd_id;
    unsigned int state = 1;

    fwk_id_is_type_ExpectAnyArgsAndReturn(false);

    status = scmi_pd_req_set_state(pd_id, state);
    TEST_ASSERT_EQUAL(status, FWK_E_DATA);
}

void test_scmi_power_domain_req_set_state_out_of_range(void)
{
    int status;
    fwk_id_t pd_id;
    unsigned int state = 1;

    fwk_id_is_type_ExpectAnyArgsAndReturn(true);
    fwk_id_get_element_idx_ExpectAnyArgsAndReturn(2);

    status = scmi_pd_req_set_state(pd_id, state);
    TEST_ASSERT_EQUAL(status, FWK_E_DATA);
}

void test_scmi_power_domain_req_set_state_send_error(void)
{
    int status;
    fwk_id_t pd_id;
    unsigned int state = 1;
    uint8_t scmi_protocol_id = (uint8_t)MOD_SCMI_PROTOCOL_ID_POWER_DOMAIN;
    uint8_t scmi_message_id = (uint8_t)MOD_SCMI_POWER_DOMAIN_REQ_STATE_SET;

    const struct scmi_pd_req_power_state_set_a2p payload = {
        .flags = dev_ctx[MOD_SCMI_POWER_DOMAIN_REQ_IDX_1].config->sync_flag,
        .power_state = state,
        .domain_id = dev_ctx[MOD_SCMI_POWER_DOMAIN_REQ_IDX_1].config->domain_id,
    };

    dev_ctx[MOD_SCMI_POWER_DOMAIN_REQ_IDX_1].is_free = true;
    fwk_id_is_type_ExpectAnyArgsAndReturn(true);
    fwk_id_get_element_idx_ExpectAnyArgsAndReturn(1);
    scmi_send_message_ExpectWithArrayAndReturn(
        scmi_message_id,
        scmi_protocol_id,
        0,
        dev_ctx[MOD_SCMI_POWER_DOMAIN_REQ_IDX_1].config->service_id,
        (const void *)&payload,
        sizeof(payload),
        sizeof(payload),
        true,
        FWK_E_DATA);

    status = scmi_pd_req_set_state(pd_id, state);
    TEST_ASSERT_EQUAL(status, FWK_E_DATA);
}

void test_scmi_power_domain_req_set_state_timer_error(void)
{
    int status;
    fwk_id_t pd_id;
    unsigned int state = 1;
    uint8_t scmi_protocol_id = (uint8_t)MOD_SCMI_PROTOCOL_ID_POWER_DOMAIN;
    uint8_t scmi_message_id = (uint8_t)MOD_SCMI_POWER_DOMAIN_REQ_STATE_SET;

    const struct scmi_pd_req_power_state_set_a2p payload = {
        .flags = dev_ctx[MOD_SCMI_POWER_DOMAIN_REQ_IDX_1].config->sync_flag,
        .power_state = state,
        .domain_id = dev_ctx[MOD_SCMI_POWER_DOMAIN_REQ_IDX_1].config->domain_id,
    };

    dev_ctx[MOD_SCMI_POWER_DOMAIN_REQ_IDX_1].is_free = true;
    dev_ctx[MOD_SCMI_POWER_DOMAIN_REQ_IDX_1].requested_state = 0;

    fwk_id_is_type_ExpectAnyArgsAndReturn(true);
    fwk_id_get_element_idx_ExpectAnyArgsAndReturn(1);
    scmi_send_message_ExpectWithArrayAndReturn(
        scmi_message_id,
        scmi_protocol_id,
        0,
        dev_ctx[MOD_SCMI_POWER_DOMAIN_REQ_IDX_1].config->service_id,
        (const void *)&payload,
        sizeof(payload),
        sizeof(payload),
        true,
        FWK_SUCCESS);
    start_alarm_api_ExpectAnyArgsAndReturn(FWK_E_ACCESS);

    status = scmi_pd_req_set_state(pd_id, state);
    TEST_ASSERT_EQUAL(status, FWK_E_ACCESS);
    TEST_ASSERT_EQUAL(dev_ctx[MOD_SCMI_POWER_DOMAIN_REQ_IDX_1].is_free, true);
    TEST_ASSERT_EQUAL(
        dev_ctx[MOD_SCMI_POWER_DOMAIN_REQ_IDX_1].requested_state, 0);
}

void test_scmi_power_domain_req_set_state(void)
{
    int status;
    fwk_id_t pd_id;
    unsigned int state = 1;
    uint8_t scmi_protocol_id = (uint8_t)MOD_SCMI_PROTOCOL_ID_POWER_DOMAIN;
    uint8_t scmi_message_id = (uint8_t)MOD_SCMI_POWER_DOMAIN_REQ_STATE_SET;

    const struct scmi_pd_req_power_state_set_a2p payload = {
        .flags = dev_ctx[MOD_SCMI_POWER_DOMAIN_REQ_IDX_1].config->sync_flag,
        .power_state = state,
        .domain_id = dev_ctx[MOD_SCMI_POWER_DOMAIN_REQ_IDX_1].config->domain_id,
    };

    dev_ctx[MOD_SCMI_POWER_DOMAIN_REQ_IDX_1].is_free = true;
    dev_ctx[MOD_SCMI_POWER_DOMAIN_REQ_IDX_1].requested_state = 0;

    fwk_id_is_type_ExpectAnyArgsAndReturn(true);
    fwk_id_get_element_idx_ExpectAnyArgsAndReturn(1);
    scmi_send_message_ExpectWithArrayAndReturn(
        scmi_message_id,
        scmi_protocol_id,
        0,
        dev_ctx[MOD_SCMI_POWER_DOMAIN_REQ_IDX_1].config->service_id,
        (const void *)&payload,
        sizeof(payload),
        sizeof(payload),
        true,
        FWK_SUCCESS);
    start_alarm_api_ExpectAnyArgsAndReturn(FWK_SUCCESS);

    status = scmi_pd_req_set_state(pd_id, state);
    TEST_ASSERT_EQUAL(status, FWK_SUCCESS);
    TEST_ASSERT_EQUAL(dev_ctx[MOD_SCMI_POWER_DOMAIN_REQ_IDX_1].is_free, false);
    TEST_ASSERT_EQUAL(
        dev_ctx[MOD_SCMI_POWER_DOMAIN_REQ_IDX_1].requested_state, 1);
}

int scmi_power_domain_req_test_main(void)
{
    UNITY_BEGIN();

    RUN_TEST(test_function_scmi_power_domain_req_init_error);
    RUN_TEST(test_function_scmi_power_domain_req_init_success);
    RUN_TEST(test_function_scmi_power_domain_req_elem_init_0);
    RUN_TEST(test_function_scmi_power_domain_req_elem_init_0_error);
    RUN_TEST(test_function_scmi_power_domain_req_elem_init_0_null_data);
    RUN_TEST(test_function_scmi_power_domain_req_elem_init_1);
    RUN_TEST(test_function_scmi_power_domain_req_elem_init_1_null_data);
    RUN_TEST(test_function_scmi_power_domain_req_bind);
    RUN_TEST(test_function_scmi_power_domain_req_bind_mod_round_0_success);
    RUN_TEST(test_function_scmi_power_domain_req_bind_mod_round_1_success);
    RUN_TEST(test_function_scmi_power_domain_req_bind_element_round_1_success);
    RUN_TEST(test_function_scmi_power_domain_req_process_bind_invalid_api);
    RUN_TEST(test_function_scmi_power_domain_req_process_bind_scmi_api_success);
    RUN_TEST(test_function_scmi_power_domain_req_process_bind_scmi_api_fail);
    RUN_TEST(test_function_scmi_power_domain_req_process_bind_PD_api);
    RUN_TEST(test_scmi_power_domain_req_get_scmi_protocol_id);
    RUN_TEST(test_scmi_power_domain_req_message_handler_range);
    RUN_TEST(test_scmi_power_domain_req_message_handler_payload);
    RUN_TEST(test_scmi_power_domain_req_message_handler_param);
    RUN_TEST(test_scmi_power_domain_req_message_handler_success);
    RUN_TEST(test_scmi_power_domain_req_get_state);
    RUN_TEST(test_scmi_power_domain_req_get_state_success);
    RUN_TEST(test_scmi_power_domain_req_set_state_send_error);
    RUN_TEST(test_scmi_power_domain_req_set_state);
    RUN_TEST(test_scmi_power_domain_req_set_state_timer_error);
    RUN_TEST(test_scmi_power_domain_req_set_state_busy);
    RUN_TEST(test_scmi_power_domain_req_set_state_incorrect_id);
    RUN_TEST(test_scmi_power_domain_req_set_state_out_of_range);
    RUN_TEST(test_scmi_power_domain_req_get_state_fail);

    return UNITY_END();
}

int main(void)
{
    return scmi_power_domain_req_test_main();
}
