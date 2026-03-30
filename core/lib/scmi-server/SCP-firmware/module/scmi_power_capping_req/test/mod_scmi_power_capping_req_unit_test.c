/*
 * Arm SCP/MCP Software
 * Copyright (c) 2025, Arm Limited and Contributors. All rights reserved.
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
#include <Mockmod_scmi_power_capping_req_extra.h>
#include <internal/Mockfwk_core_internal.h>

#include <mod_scmi.h>

#include <interface_power_management.h>

#include <fwk_element.h>
#include <fwk_macros.h>

#include UNIT_TEST_SRC

enum scp_pcap_req_nums {
    MOD_SCMI_POWER_CAPPING_REQ_IDX_0,
    MOD_SCMI_POWER_CAPPING_REQ_IDX_1,
    MOD_SCMI_POWER_CAPPING_REQ_COUNT,
};

/*
 * Power Capping Req module config
 */
static const struct fwk_element
    power_capping_req_element_table[MOD_SCMI_POWER_CAPPING_REQ_COUNT + 1] = {
    [MOD_SCMI_POWER_CAPPING_REQ_IDX_0] = {
        .name = "Fake Power Capping Req",
        .data = &((struct mod_scmi_power_capping_req_dev_config){
            .service_id = FWK_ID_ELEMENT_INIT(
                FWK_MODULE_IDX_SCMI,
                0),
            .alarm_id = FWK_ID_SUB_ELEMENT_INIT(FWK_MODULE_IDX_TIMER, MOD_SCMI_POWER_CAPPING_REQ_IDX_0, 0),
            .alarm_delay = 10,
        }),
    },
    [MOD_SCMI_POWER_CAPPING_REQ_IDX_1] = {
        .name = "Fake Power Capping Req",
        .data = &((struct mod_scmi_power_capping_req_dev_config){
            .service_id = FWK_ID_ELEMENT_INIT(
                FWK_MODULE_IDX_SCMI,
                0),
            .alarm_id = FWK_ID_SUB_ELEMENT_INIT(FWK_MODULE_IDX_TIMER, MOD_SCMI_POWER_CAPPING_REQ_IDX_1, 0),
            .alarm_delay = 10,
        }),
    },
    [MOD_SCMI_POWER_CAPPING_REQ_COUNT] = { 0 },
};

static const struct fwk_element *get_power_capping_req_element_table(
    fwk_id_t module_id)
{
    return power_capping_req_element_table;
}

const struct fwk_module_config config_scmi_power_capping_req = {
    .elements =
        FWK_MODULE_DYNAMIC_ELEMENTS(get_power_capping_req_element_table),
};

const struct mod_scmi_from_protocol_req_api scmi_api = {
    .scmi_send_message = scmi_send_message,
    .response_message_handler = response_message_handler,
};

static struct mod_timer_alarm_api alarm_api = {
    .start = start_alarm_api,
    .stop = stop_alarm_api,
};

static struct scmi_power_capping_req_dev_ctx
    dev_ctx[MOD_SCMI_POWER_CAPPING_REQ_COUNT];

void setUp(void)
{
    memset(&mod_ctx, 0, sizeof(mod_ctx));

    memset(
        &dev_ctx[MOD_SCMI_POWER_CAPPING_REQ_IDX_0],
        0,
        sizeof(struct scmi_power_capping_req_dev_ctx));
    dev_ctx[MOD_SCMI_POWER_CAPPING_REQ_IDX_0].config =
        (const struct mod_scmi_power_capping_req_dev_config *)
            power_capping_req_element_table[MOD_SCMI_POWER_CAPPING_REQ_IDX_0]
                .data;

    memset(
        &dev_ctx[MOD_SCMI_POWER_CAPPING_REQ_IDX_1],
        0,
        sizeof(struct scmi_power_capping_req_dev_ctx));
    dev_ctx[MOD_SCMI_POWER_CAPPING_REQ_IDX_1].config =
        (const struct mod_scmi_power_capping_req_dev_config *)
            power_capping_req_element_table[MOD_SCMI_POWER_CAPPING_REQ_IDX_1]
                .data;

    mod_ctx.dev_ctx_table = dev_ctx;
    mod_ctx.element_count = MOD_SCMI_POWER_CAPPING_REQ_COUNT;
    mod_ctx.scmi_api = &scmi_api;
    mod_ctx.alarm_api = &alarm_api;

    handler_table[MOD_SCMI_POWER_CAPPING_REQ_CAP_SET] = fake_message_handler;
}

void tearDown(void)
{
}

void test_UT(void)
{
    int status;
    status = FWK_SUCCESS;
    TEST_ASSERT_EQUAL(status, FWK_SUCCESS);
}

void test_scmi_power_capping_req_elem_init_0(void)
{
    int status;
    fwk_id_t element_id = FWK_ID_ELEMENT(
        FWK_MODULE_IDX_SCMI_POWER_CAPPING_REQ,
        MOD_SCMI_POWER_CAPPING_REQ_IDX_0);

    fwk_id_get_element_idx_ExpectAndReturn(
        element_id, MOD_SCMI_POWER_CAPPING_REQ_IDX_0);
    fwk_id_get_element_idx_ExpectAndReturn(
        element_id, MOD_SCMI_POWER_CAPPING_REQ_IDX_0);

    fwk_optional_id_is_defined_ExpectAnyArgsAndReturn(true);

    fwk_module_is_valid_sub_element_id_ExpectAnyArgsAndReturn(true);

    status = scmi_power_capping_req_elem_init(
        element_id,
        0,
        power_capping_req_element_table[MOD_SCMI_POWER_CAPPING_REQ_IDX_0].data);
    TEST_ASSERT_EQUAL(status, FWK_SUCCESS);
}

void test_scmi_power_capping_req_elem_init_1(void)
{
    int status;
    fwk_id_t element_id = FWK_ID_ELEMENT(
        FWK_MODULE_IDX_SCMI_POWER_CAPPING_REQ,
        MOD_SCMI_POWER_CAPPING_REQ_IDX_1);

    fwk_id_get_element_idx_ExpectAndReturn(
        element_id, MOD_SCMI_POWER_CAPPING_REQ_IDX_1);
    fwk_id_get_element_idx_ExpectAndReturn(
        element_id, MOD_SCMI_POWER_CAPPING_REQ_IDX_1);

    fwk_optional_id_is_defined_ExpectAnyArgsAndReturn(true);

    fwk_module_is_valid_sub_element_id_ExpectAnyArgsAndReturn(true);

    status = scmi_power_capping_req_elem_init(
        element_id,
        0,
        power_capping_req_element_table[MOD_SCMI_POWER_CAPPING_REQ_IDX_1].data);
    TEST_ASSERT_EQUAL(status, FWK_SUCCESS);
}

void test_scmi_power_capping_req_elem_init_1_not_defined(void)
{
    int status;
    fwk_id_t element_id = FWK_ID_ELEMENT(
        FWK_MODULE_IDX_SCMI_POWER_CAPPING_REQ,
        MOD_SCMI_POWER_CAPPING_REQ_IDX_1);

    fwk_id_get_element_idx_ExpectAndReturn(
        element_id, MOD_SCMI_POWER_CAPPING_REQ_IDX_1);
    fwk_id_get_element_idx_ExpectAndReturn(
        element_id, MOD_SCMI_POWER_CAPPING_REQ_IDX_1);

    fwk_optional_id_is_defined_ExpectAnyArgsAndReturn(false);
    status = scmi_power_capping_req_elem_init(
        element_id,
        0,
        power_capping_req_element_table[MOD_SCMI_POWER_CAPPING_REQ_IDX_1].data);
    TEST_ASSERT_EQUAL(status, FWK_SUCCESS);
}

void test_scmi_power_capping_req_elem_init_invalid_id(void)
{
    int status;
    fwk_id_t element_id = FWK_ID_ELEMENT(
        FWK_MODULE_IDX_SCMI_POWER_CAPPING_REQ,
        MOD_SCMI_POWER_CAPPING_REQ_COUNT);

    fwk_id_get_element_idx_ExpectAnyArgsAndReturn(
        MOD_SCMI_POWER_CAPPING_REQ_COUNT);

    status = scmi_power_capping_req_elem_init(
        element_id,
        0,
        power_capping_req_element_table[MOD_SCMI_POWER_CAPPING_REQ_IDX_1].data);
    TEST_ASSERT_EQUAL(status, FWK_E_PARAM);
}

void test_scmi_power_capping_req_elem_init_null_data(void)
{
    int status;
    fwk_id_t element_id = FWK_ID_ELEMENT(
        FWK_MODULE_IDX_SCMI_POWER_CAPPING_REQ,
        MOD_SCMI_POWER_CAPPING_REQ_IDX_1);

    fwk_id_get_element_idx_ExpectAnyArgsAndReturn(
        MOD_SCMI_POWER_CAPPING_REQ_IDX_1);
    fwk_id_get_element_idx_ExpectAnyArgsAndReturn(
        MOD_SCMI_POWER_CAPPING_REQ_IDX_1);

    status = scmi_power_capping_req_elem_init(element_id, 0, NULL);
    TEST_ASSERT_EQUAL(status, FWK_E_PANIC);
}

void test_scmi_power_capping_req_init_success(void)
{
    int status;

    fwk_mm_calloc_ExpectAndReturn(
        MOD_SCMI_POWER_CAPPING_REQ_COUNT,
        sizeof(struct scmi_power_capping_req_dev_ctx),
        (void *)dev_ctx);

    status = scmi_power_capping_req_init(
        fwk_module_id_scmi_power_capping_req,
        MOD_SCMI_POWER_CAPPING_REQ_COUNT,
        NULL);
    TEST_ASSERT_EQUAL(status, FWK_SUCCESS);
    TEST_ASSERT_EQUAL(mod_ctx.element_count, MOD_SCMI_POWER_CAPPING_REQ_COUNT);
    TEST_ASSERT_EQUAL(mod_ctx.dev_ctx_table, dev_ctx);
}

void test_scmi_power_capping_req_init_no_elements(void)
{
    int status;

    status = scmi_power_capping_req_init(
        fwk_module_id_scmi_power_capping_req, 0, NULL);
    TEST_ASSERT_EQUAL(status, FWK_E_SUPPORT);
}

void test_scmi_power_capping_req_set_power_cap_error_id_type(void)
{
    int status;
    fwk_id_t element_id = FWK_ID_ELEMENT(
        FWK_MODULE_IDX_SCMI_POWER_CAPPING_REQ,
        MOD_SCMI_POWER_CAPPING_REQ_IDX_0);

    const struct scmi_pcap_req_set_cap_a2p payload = {
        .power_cap = 10, /* Nominal values */
        .flags = 0xAA,
    };

    fwk_id_is_type_ExpectAnyArgsAndReturn(false);

    status = set_power_cap(element_id, payload.power_cap, payload.flags);
    TEST_ASSERT_EQUAL(status, FWK_E_PARAM);
}

void test_scmi_power_capping_req_set_cap(void)
{
    int status;
    fwk_id_t element_id;

    const struct scmi_pcap_req_set_cap_a2p payload = {
        .power_cap = 10, /* Nominal values */
        .flags = 0xAA,
    };

    uint8_t scmi_protocol_id = (uint8_t)MOD_SCMI_PROTOCOL_ID_POWER_CAPPING;
    uint8_t scmi_message_id = (uint8_t)MOD_SCMI_POWER_CAPPING_REQ_CAP_SET;

    fwk_id_is_type_ExpectAnyArgsAndReturn(true);
    fwk_id_get_element_idx_ExpectAnyArgsAndReturn(
        MOD_SCMI_POWER_CAPPING_REQ_IDX_0);

    mod_ctx.dev_ctx_table[MOD_SCMI_POWER_CAPPING_REQ_IDX_0].responded = true;

    scmi_send_message_ExpectWithArrayAndReturn(
        scmi_message_id,
        scmi_protocol_id,
        0,
        dev_ctx[MOD_SCMI_POWER_CAPPING_REQ_IDX_0].config->service_id,
        (const void *)&payload,
        sizeof(payload),
        sizeof(payload),
        true,
        FWK_SUCCESS);

    fwk_id_get_element_idx_ExpectAnyArgsAndReturn(
        MOD_SCMI_POWER_CAPPING_REQ_IDX_0);
    start_alarm_api_ExpectAnyArgsAndReturn(FWK_SUCCESS);
    fwk_id_get_element_idx_ExpectAnyArgsAndReturn(
        MOD_SCMI_POWER_CAPPING_REQ_IDX_0);

    status = set_power_cap(element_id, payload.power_cap, payload.flags);
    TEST_ASSERT_EQUAL(status, FWK_SUCCESS);
}

void test_scmi_power_capping_req_set_cap_busy(void)
{
    int status;
    fwk_id_t element_id;

    const struct scmi_pcap_req_set_cap_a2p payload = {
        .power_cap = 10, /* Nominal values */
        .flags = 0xAA,
    };

    fwk_id_is_type_ExpectAnyArgsAndReturn(true);
    fwk_id_get_element_idx_ExpectAnyArgsAndReturn(
        MOD_SCMI_POWER_CAPPING_REQ_IDX_0);

    mod_ctx.dev_ctx_table[MOD_SCMI_POWER_CAPPING_REQ_IDX_0].responded = false;

    status = set_power_cap(element_id, payload.power_cap, payload.flags);
    TEST_ASSERT_EQUAL(status, FWK_E_BUSY);
}

void test_scmi_power_capping_req_message_handler_range(void)
{
    int status;
    fwk_id_t expected_service_id;
    fwk_id_t protocol_id;
    uint8_t payload[sizeof(struct scmi_pcap_req_set_cap_a2p)] = { 0 };

    expected_service_id =
        dev_ctx[MOD_SCMI_POWER_CAPPING_REQ_IDX_0].config->service_id;

    status = scmi_power_capping_req_message_handler(
        protocol_id,
        expected_service_id,
        (uint32_t *)payload,
        sizeof(payload),
        sizeof(handler_table));
    TEST_ASSERT_EQUAL(status, FWK_E_RANGE);
}

void test_scmi_power_capping_req_message_handler_invalid_param(void)
{
    int status;
    fwk_id_t expected_service_id;
    fwk_id_t protocol_id;
    uint8_t payload[sizeof(struct scmi_pcap_req_set_cap_a2p)] = { 0 };

    expected_service_id =
        dev_ctx[MOD_SCMI_POWER_CAPPING_REQ_IDX_0].config->service_id;

    status = scmi_power_capping_req_message_handler(
        protocol_id,
        expected_service_id,
        (uint32_t *)payload,
        sizeof(payload) + 1,
        MOD_SCMI_POWER_CAPPING_REQ_CAP_SET);
    TEST_ASSERT_EQUAL(status, FWK_E_PARAM);
}

void test_scmi_power_capping_req_message_handler_success(void)
{
    int status;
    fwk_id_t expected_service_id;
    fwk_id_t protocol_id;
    uint8_t payload[sizeof(struct scmi_pcap_req_set_cap_a2p)] = { 0 };

    expected_service_id =
        dev_ctx[MOD_SCMI_POWER_CAPPING_REQ_IDX_0].config->service_id;

    fake_message_handler_ExpectWithArrayAndReturn(
        expected_service_id,
        (uint32_t *)payload,
        sizeof(payload),
        sizeof(payload),
        FWK_SUCCESS);
    response_message_handler_ExpectAndReturn(expected_service_id, FWK_SUCCESS);

    status = scmi_power_capping_req_message_handler(
        protocol_id,
        expected_service_id,
        (uint32_t *)payload,
        sizeof(payload),
        MOD_SCMI_POWER_CAPPING_REQ_CAP_SET);
    TEST_ASSERT_EQUAL(status, FWK_SUCCESS);
}

void test_scmi_power_capping_req_bind_0_success(void)
{
    int status;

    fwk_id_is_type_ExpectAnyArgsAndReturn(true);
    fwk_module_bind_ExpectAndReturn(
        FWK_ID_MODULE(FWK_MODULE_IDX_SCMI),
        FWK_ID_API(FWK_MODULE_IDX_SCMI, MOD_SCMI_API_IDX_PROTOCOL_REQ),
        &mod_ctx.scmi_api,
        FWK_SUCCESS);

    fwk_id_get_element_idx_ExpectAnyArgsAndReturn(
        MOD_SCMI_POWER_CAPPING_REQ_IDX_0);
    fwk_optional_id_is_defined_ExpectAnyArgsAndReturn(true);

    fwk_module_bind_ExpectAndReturn(
        dev_ctx[MOD_SCMI_POWER_CAPPING_REQ_IDX_0].config->alarm_id,
        MOD_TIMER_API_ID_ALARM,
        &mod_ctx.alarm_api,
        FWK_SUCCESS);

    status =
        scmi_power_capping_req_bind(fwk_module_id_scmi_power_capping_req, 0);
    TEST_ASSERT_EQUAL(status, FWK_SUCCESS);
}

void test_scmi_power_capping_req_bind_1_success(void)
{
    int status;

    fwk_id_get_element_idx_ExpectAnyArgsAndReturn(0);
    fwk_optional_id_is_defined_ExpectAnyArgsAndReturn(true);

    fwk_module_bind_ExpectAndReturn(
        dev_ctx[MOD_SCMI_POWER_CAPPING_REQ_IDX_0].config->alarm_id,
        MOD_TIMER_API_ID_ALARM,
        &mod_ctx.alarm_api,
        FWK_SUCCESS);

    status =
        scmi_power_capping_req_bind(fwk_module_id_scmi_power_capping_req, 1);
    TEST_ASSERT_EQUAL(status, FWK_SUCCESS);
}

void test_scmi_power_capping_req_bind_0_error(void)
{
    int status;

    fwk_id_is_type_ExpectAnyArgsAndReturn(true);
    fwk_module_bind_ExpectAndReturn(
        FWK_ID_MODULE(FWK_MODULE_IDX_SCMI),
        FWK_ID_API(FWK_MODULE_IDX_SCMI, MOD_SCMI_API_IDX_PROTOCOL_REQ),
        &mod_ctx.scmi_api,
        FWK_E_PARAM);

    fwk_id_get_element_idx_ExpectAnyArgsAndReturn(0);
    fwk_optional_id_is_defined_ExpectAnyArgsAndReturn(true);

    status =
        scmi_power_capping_req_bind(fwk_module_id_scmi_power_capping_req, 0);
    TEST_ASSERT_EQUAL(status, FWK_E_PARAM);
}

void test_scmi_power_capping_req_bind_req_success(void)
{
    int status;
    fwk_id_t target_id;
    struct mod_power_capping_req_api_id *api = NULL;

    /* PCAP Req API */
    fwk_id_get_api_idx_ExpectAnyArgsAndReturn(MOD_POW_CAP_REQ_API_IDX_REQ);

    status = scmi_power_capping_req_process_bind_request(
        fwk_module_id_scmi,
        target_id,
        mod_power_capping_req_api_id,
        (const void **)&api);

    TEST_ASSERT_EQUAL(status, FWK_SUCCESS);
    TEST_ASSERT_EQUAL(api, &power_capping_req_api);

    /* SCMI API */
    fwk_id_get_api_idx_ExpectAnyArgsAndReturn(MOD_POW_CAP_REQ_API_IDX_SCMI_REQ);
    fwk_id_is_equal_ExpectAnyArgsAndReturn(true);
    fwk_id_build_module_id_ExpectAnyArgsAndReturn(fwk_module_id_scmi);
    api = NULL;

    status = scmi_power_capping_req_process_bind_request(
        fwk_module_id_scmi,
        target_id,
        mod_power_capping_req_api_id,
        (const void **)&api);

    TEST_ASSERT_EQUAL(status, FWK_SUCCESS);
    TEST_ASSERT_EQUAL(api, &scmi_power_capping_req_scmi_to_protocol_api);

    /* Power Management API */
    fwk_id_get_api_idx_ExpectAnyArgsAndReturn(
        MOD_POW_CAP_REQ_API_IDX_LIMITER_POWER_API);
    api = NULL;

    status = scmi_power_capping_req_process_bind_request(
        fwk_module_id_scmi,
        target_id,
        mod_power_capping_req_api_id,
        (const void **)&api);

    TEST_ASSERT_EQUAL(status, FWK_SUCCESS);
    TEST_ASSERT_EQUAL(api, &power_management_api);
}

void test_scmi_power_capping_req_bind_req_error_missing_id(void)
{
    int status;
    fwk_id_t target_id;
    struct mod_power_capping_req_api_id *api = NULL;

    fwk_id_get_api_idx_ExpectAnyArgsAndReturn(MOD_POW_CAP_REQ_API_IDX_COUNT);

    status = scmi_power_capping_req_process_bind_request(
        fwk_module_id_scmi,
        target_id,
        mod_power_capping_req_api_id,
        (const void **)&api);

    TEST_ASSERT_EQUAL(status, FWK_E_SUPPORT);
    TEST_ASSERT_EQUAL(api, NULL);
}

void test_scmi_power_capping_req_bind_req_error_module_id(void)
{
    int status;
    fwk_id_t invalid_id = { .value = UINT32_MAX };
    fwk_id_t target_id;
    struct mod_power_capping_req_api_id *api = NULL;

    fwk_id_get_api_idx_ExpectAnyArgsAndReturn(MOD_POW_CAP_REQ_API_IDX_SCMI_REQ);
    fwk_id_is_equal_ExpectAnyArgsAndReturn(false);
    fwk_id_build_module_id_ExpectAnyArgsAndReturn(invalid_id);

    status = scmi_power_capping_req_process_bind_request(
        fwk_module_id_scmi,
        target_id,
        mod_power_capping_req_api_id,
        (const void **)&api);

    TEST_ASSERT_EQUAL(status, FWK_E_ACCESS);
    TEST_ASSERT_EQUAL(api, NULL);
}

int scmi_power_capping_req_test_main(void)
{
    UNITY_BEGIN();

    RUN_TEST(test_scmi_power_capping_req_elem_init_0);
    RUN_TEST(test_scmi_power_capping_req_elem_init_1);
    RUN_TEST(test_scmi_power_capping_req_elem_init_1_not_defined);
    RUN_TEST(test_scmi_power_capping_req_elem_init_invalid_id);
    RUN_TEST(test_scmi_power_capping_req_elem_init_null_data);
    RUN_TEST(test_scmi_power_capping_req_init_success);
    RUN_TEST(test_scmi_power_capping_req_init_no_elements);
    RUN_TEST(test_scmi_power_capping_req_set_power_cap_error_id_type);
    RUN_TEST(test_scmi_power_capping_req_set_cap);
    RUN_TEST(test_scmi_power_capping_req_set_cap_busy);
    RUN_TEST(test_scmi_power_capping_req_message_handler_range);
    RUN_TEST(test_scmi_power_capping_req_message_handler_invalid_param);
    RUN_TEST(test_scmi_power_capping_req_message_handler_success);
    RUN_TEST(test_scmi_power_capping_req_bind_0_success);
    RUN_TEST(test_scmi_power_capping_req_bind_1_success);
    RUN_TEST(test_scmi_power_capping_req_bind_0_error);
    RUN_TEST(test_scmi_power_capping_req_bind_req_success);
    RUN_TEST(test_scmi_power_capping_req_bind_req_error_missing_id);
    RUN_TEST(test_scmi_power_capping_req_bind_req_error_module_id);

    return UNITY_END();
}

int main(void)
{
    return scmi_power_capping_req_test_main();
}
