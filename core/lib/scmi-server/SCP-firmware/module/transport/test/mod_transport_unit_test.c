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
#    include <Mockfwk_module.h>
#endif

#include <internal/transport.h>

#include <mod_transport.h>

#include <fwk_element.h>
#include <fwk_macros.h>

#include UNIT_TEST_SRC

#define BUILD_HAS_BASE_PROTOCOL

#ifndef BUILD_HAS_INBAND_MSG_SUPPORT
#    error Unit test requires BUILD_HAS_INBAND_MSG_SUPPORT
#endif
#ifndef BUILD_HAS_OUTBAND_MSG_SUPPORT
#    error Unit test requires BUILD_HAS_OUTBAND_MSG_SUPPORT
#endif

#define FAKE_MODULE_ID    0x5
#define TEST_MAILBOX_SIZE 100

enum fake_services {
    FAKE_SERVICE_IDX_OUT_BAND_TEST_CHANNEL,
    FAKE_SERVICE_IDX_IN_BAND_TEST_CHANNEL,
    FAKE_SERVICE_IDX_COUNT,
};

int mock_send_message(struct mod_transport_buffer *message, fwk_id_t device_id)
{
    return FWK_SUCCESS;
}

int mock_get_message(struct mod_transport_buffer *message, fwk_id_t device_id)
{
    return FWK_SUCCESS;
}

int mock_trigger_event(fwk_id_t device_id)
{
    return FWK_SUCCESS;
}

struct mod_transport_driver_api mock_mod_transport_driver_api = {
    .send_message = mock_send_message,
    .get_message = mock_get_message,
    .trigger_event = mock_trigger_event,
};

void setUp(void)
{
    fwk_id_t module_id = FWK_ID_MODULE_INIT(FAKE_MODULE_ID);
    fwk_id_t ib_channel_id = FWK_ID_ELEMENT_INIT(
        FAKE_MODULE_ID, FAKE_SERVICE_IDX_IN_BAND_TEST_CHANNEL);
    fwk_id_t ob_channel_id = FWK_ID_ELEMENT_INIT(
        FAKE_MODULE_ID, FAKE_SERVICE_IDX_OUT_BAND_TEST_CHANNEL);

    transport_init(module_id, FAKE_SERVICE_IDX_COUNT, NULL);

    fwk_id_get_element_idx_ExpectAndReturn(
        ib_channel_id, FAKE_SERVICE_IDX_IN_BAND_TEST_CHANNEL);
    static const struct mod_transport_channel_config ib_config = {
        .transport_type = MOD_TRANSPORT_CHANNEL_TRANSPORT_TYPE_IN_BAND,
        .channel_type = MOD_TRANSPORT_CHANNEL_TYPE_REQUESTER,
        .in_band_mailbox_size = TEST_MAILBOX_SIZE
    };
    transport_channel_init(ib_channel_id, 0, &ib_config);

    uintptr_t out_band_mailbox = (uintptr_t)fwk_mm_alloc(1, TEST_MAILBOX_SIZE);
    fwk_id_get_element_idx_ExpectAndReturn(
        ob_channel_id, FAKE_SERVICE_IDX_OUT_BAND_TEST_CHANNEL);
    static struct mod_transport_channel_config ob_config = {
        .transport_type = MOD_TRANSPORT_CHANNEL_TRANSPORT_TYPE_OUT_BAND,
        .channel_type = MOD_TRANSPORT_CHANNEL_TYPE_REQUESTER,
        .out_band_mailbox_size = TEST_MAILBOX_SIZE,
    };
    ob_config.out_band_mailbox_address = out_band_mailbox;
    transport_channel_init(ob_channel_id, 0, &ob_config);

    struct transport_channel_ctx *channel_ctx;
    channel_ctx =
        &transport_ctx.channel_ctx_table[FAKE_SERVICE_IDX_IN_BAND_TEST_CHANNEL];
    channel_ctx->driver_api = &mock_mod_transport_driver_api;
    channel_ctx =
        &transport_ctx
             .channel_ctx_table[FAKE_SERVICE_IDX_OUT_BAND_TEST_CHANNEL];
    channel_ctx->driver_api = &mock_mod_transport_driver_api;
}

void tearDown(void)
{
}

void test_transport_payload_size(void)
{
    struct transport_channel_ctx *channel_ctx =
        &transport_ctx.channel_ctx_table[FAKE_SERVICE_IDX_IN_BAND_TEST_CHANNEL];

    TEST_ASSERT_EQUAL(
        TEST_MAILBOX_SIZE - sizeof(struct mod_transport_buffer),
        channel_ctx->max_payload_size);
}

void test_transport_write_payload_invalid_state_unlocked(void)
{
    int status;

    fwk_id_t service_id = FWK_ID_ELEMENT_INIT(
        FAKE_MODULE_ID, FAKE_SERVICE_IDX_IN_BAND_TEST_CHANNEL);
    char string[] = "Test";

    /* We are not going through a proper message cycle so let's set the lock */
    struct transport_channel_ctx *channel_ctx =
        &transport_ctx.channel_ctx_table[FAKE_SERVICE_IDX_IN_BAND_TEST_CHANNEL];
    channel_ctx->locked = false;

    fwk_id_get_element_idx_ExpectAndReturn(
        service_id, FAKE_SERVICE_IDX_IN_BAND_TEST_CHANNEL);

    status = transport_write_payload(service_id, 0, string, sizeof(string));
    TEST_ASSERT_EQUAL(FWK_E_ACCESS, status);
}

void test_transport_write_payload_invalid_param_null_payload(void)
{
    int status;

    fwk_id_t service_id = FWK_ID_ELEMENT_INIT(
        FAKE_MODULE_ID, FAKE_SERVICE_IDX_IN_BAND_TEST_CHANNEL);
    char string[] = "Test";

    /* We are not going through a proper message cycle so let's set the lock */
    struct transport_channel_ctx *channel_ctx =
        &transport_ctx.channel_ctx_table[FAKE_SERVICE_IDX_IN_BAND_TEST_CHANNEL];
    channel_ctx->locked = true;

    fwk_id_get_element_idx_ExpectAndReturn(
        service_id, FAKE_SERVICE_IDX_IN_BAND_TEST_CHANNEL);

    status = transport_write_payload(service_id, 0, NULL, sizeof(string));
    TEST_ASSERT_EQUAL(FWK_E_PARAM, status);
}

void test_transport_write_payload_invalid_param_large_size(void)
{
    int status;
    size_t offset = 50;

    fwk_id_t service_id = FWK_ID_ELEMENT_INIT(
        FAKE_MODULE_ID, FAKE_SERVICE_IDX_IN_BAND_TEST_CHANNEL);
    char payload[] = "Test";

    /* We are not going through a proper message cycle so let's set the lock */
    struct transport_channel_ctx *channel_ctx =
        &transport_ctx.channel_ctx_table[FAKE_SERVICE_IDX_IN_BAND_TEST_CHANNEL];
    channel_ctx->locked = true;

    fwk_id_get_element_idx_ExpectAndReturn(
        service_id, FAKE_SERVICE_IDX_IN_BAND_TEST_CHANNEL);

    status = transport_write_payload(
        service_id, offset, payload, SIZE_MAX - (offset / 2));
    TEST_ASSERT_EQUAL(FWK_E_PARAM, status);
}

void test_transport_write_payload_invalid_param_large_offset(void)
{
    int status;

    fwk_id_t service_id = FWK_ID_ELEMENT_INIT(
        FAKE_MODULE_ID, FAKE_SERVICE_IDX_IN_BAND_TEST_CHANNEL);
    char payload[] = "Test";

    /* We are not going through a proper message cycle so let's set the lock */
    struct transport_channel_ctx *channel_ctx =
        &transport_ctx.channel_ctx_table[FAKE_SERVICE_IDX_IN_BAND_TEST_CHANNEL];
    channel_ctx->locked = true;

    fwk_id_get_element_idx_ExpectAndReturn(
        service_id, FAKE_SERVICE_IDX_IN_BAND_TEST_CHANNEL);

    status = transport_write_payload(
        service_id, SIZE_MAX - 2, payload, sizeof(payload));
    TEST_ASSERT_EQUAL(FWK_E_PARAM, status);
}

void test_transport_write_payload_invalid_param_offset_beyond_end(void)
{
    int status;

    fwk_id_t service_id = FWK_ID_ELEMENT_INIT(
        FAKE_MODULE_ID, FAKE_SERVICE_IDX_IN_BAND_TEST_CHANNEL);
    char payload[] = "Test";

    /* We are not going through a proper message cycle so let's set the lock */
    struct transport_channel_ctx *channel_ctx =
        &transport_ctx.channel_ctx_table[FAKE_SERVICE_IDX_IN_BAND_TEST_CHANNEL];
    channel_ctx->locked = true;

    fwk_id_get_element_idx_ExpectAndReturn(
        service_id, FAKE_SERVICE_IDX_IN_BAND_TEST_CHANNEL);

    status = transport_write_payload(
        service_id, channel_ctx->max_payload_size, payload, 1);
    TEST_ASSERT_EQUAL(FWK_E_PARAM, status);
}

void test_transport_write_payload_valid_param_start(void)
{
    int status;

    fwk_id_t service_id = FWK_ID_ELEMENT_INIT(
        FAKE_MODULE_ID, FAKE_SERVICE_IDX_IN_BAND_TEST_CHANNEL);
    char payload[] = "Test";

    /* We are not going through a proper message cycle so let's set the lock */
    struct transport_channel_ctx *channel_ctx =
        &transport_ctx.channel_ctx_table[FAKE_SERVICE_IDX_IN_BAND_TEST_CHANNEL];
    channel_ctx->locked = true;

    fwk_id_get_element_idx_ExpectAndReturn(
        service_id, FAKE_SERVICE_IDX_IN_BAND_TEST_CHANNEL);

    status = transport_write_payload(service_id, 0, payload, sizeof(payload));
    TEST_ASSERT_EQUAL(FWK_SUCCESS, status);

    uint8_t *dst_payload = (uint8_t *)channel_ctx->out->payload;
    for (size_t i = 0; i < sizeof(payload); i++) {
        TEST_ASSERT_EQUAL(payload[i], dst_payload[i]);
    }
}

void test_transport_write_payload_valid_param_end(void)
{
    int status;

    fwk_id_t service_id = FWK_ID_ELEMENT_INIT(
        FAKE_MODULE_ID, FAKE_SERVICE_IDX_IN_BAND_TEST_CHANNEL);
    char payload[] = "T";

    /* We are not going through a proper message cycle so let's set the lock */
    struct transport_channel_ctx *channel_ctx =
        &transport_ctx.channel_ctx_table[FAKE_SERVICE_IDX_IN_BAND_TEST_CHANNEL];
    channel_ctx->locked = true;

    fwk_id_get_element_idx_ExpectAndReturn(
        service_id, FAKE_SERVICE_IDX_IN_BAND_TEST_CHANNEL);

    status = transport_write_payload(
        service_id, channel_ctx->max_payload_size - 1, payload, 1);
    TEST_ASSERT_EQUAL(FWK_SUCCESS, status);

    uint8_t *dst_payload = (uint8_t *)channel_ctx->out->payload;
    TEST_ASSERT_EQUAL(
        payload[0], dst_payload[channel_ctx->max_payload_size - 1]);
}

void test_transport_respond_invalid_state_null_buffer_ib(void)
{
    int status;

    fwk_id_t service_id = FWK_ID_ELEMENT_INIT(
        FAKE_MODULE_ID, FAKE_SERVICE_IDX_IN_BAND_TEST_CHANNEL);
    char payload[] = "Test";

    struct transport_channel_ctx *channel_ctx =
        &transport_ctx.channel_ctx_table[FAKE_SERVICE_IDX_IN_BAND_TEST_CHANNEL];
    channel_ctx->locked = true;

    channel_ctx->out = NULL;

    fwk_id_get_element_idx_ExpectAndReturn(
        service_id, FAKE_SERVICE_IDX_IN_BAND_TEST_CHANNEL);

    status = transport_respond(service_id, payload, sizeof(payload));

    TEST_ASSERT_EQUAL(FWK_E_PANIC, status);
    TEST_ASSERT_EQUAL(true, channel_ctx->locked);
}

void test_transport_respond_invalid_state_null_buffer_ob(void)
{
    int status;

    fwk_id_t service_id = FWK_ID_ELEMENT_INIT(
        FAKE_MODULE_ID, FAKE_SERVICE_IDX_OUT_BAND_TEST_CHANNEL);
    char payload[] = "Test";

    struct transport_channel_ctx *channel_ctx =
        &transport_ctx
             .channel_ctx_table[FAKE_SERVICE_IDX_OUT_BAND_TEST_CHANNEL];
    channel_ctx->locked = true;

    channel_ctx->config->out_band_mailbox_address = (uintptr_t)NULL;

    fwk_id_get_element_idx_ExpectAndReturn(
        service_id, FAKE_SERVICE_IDX_OUT_BAND_TEST_CHANNEL);

    status = transport_respond(service_id, payload, sizeof(payload));

    TEST_ASSERT_EQUAL(FWK_E_PANIC, status);
    TEST_ASSERT_EQUAL(true, channel_ctx->locked);
}

void test_transport_respond_invalid_param_large_size(void)
{
    int status;

    fwk_id_t service_id = FWK_ID_ELEMENT_INIT(
        FAKE_MODULE_ID, FAKE_SERVICE_IDX_IN_BAND_TEST_CHANNEL);
    char payload[] = "Test";

    struct transport_channel_ctx *channel_ctx =
        &transport_ctx.channel_ctx_table[FAKE_SERVICE_IDX_IN_BAND_TEST_CHANNEL];
    channel_ctx->locked = true;

    fwk_id_get_element_idx_ExpectAndReturn(
        service_id, FAKE_SERVICE_IDX_IN_BAND_TEST_CHANNEL);

    status = transport_respond(
        service_id, payload, channel_ctx->max_payload_size + 1);

    TEST_ASSERT_EQUAL(FWK_E_PARAM, status);
}

void test_transport_respond_invalid_param_null_payload_ib(void)
{
    int status;

    fwk_id_t service_id = FWK_ID_ELEMENT_INIT(
        FAKE_MODULE_ID, FAKE_SERVICE_IDX_IN_BAND_TEST_CHANNEL);

    struct transport_channel_ctx *channel_ctx =
        &transport_ctx.channel_ctx_table[FAKE_SERVICE_IDX_IN_BAND_TEST_CHANNEL];
    channel_ctx->locked = true;

    uint8_t *dst_payload = (uint8_t *)channel_ctx->out->payload;
    char payload[] = "Test";
    memcpy(dst_payload, payload, sizeof(payload));

    fwk_id_get_element_idx_ExpectAndReturn(
        service_id, FAKE_SERVICE_IDX_IN_BAND_TEST_CHANNEL);

    status = transport_respond(service_id, NULL, sizeof(payload));

    TEST_ASSERT_EQUAL(FWK_SUCCESS, status);

    /* Destination payload should be untouched. */
    for (size_t i = 0; i < sizeof(payload); i++) {
        TEST_ASSERT_EQUAL(payload[i], dst_payload[i]);
    }
}

void test_transport_respond_invalid_param_null_payload_ob(void)
{
    int status;

    fwk_id_t service_id = FWK_ID_ELEMENT_INIT(
        FAKE_MODULE_ID, FAKE_SERVICE_IDX_OUT_BAND_TEST_CHANNEL);

    struct transport_channel_ctx *channel_ctx =
        &transport_ctx
             .channel_ctx_table[FAKE_SERVICE_IDX_OUT_BAND_TEST_CHANNEL];
    channel_ctx->locked = true;

    struct mod_transport_buffer *buffer = NULL;
    buffer = ((struct mod_transport_buffer *)
                  channel_ctx->config->out_band_mailbox_address);

    /* When in out-band mode and given a NULL buffer transport_respond copies
     * from the out buffer */
    uint8_t *src_payload = (uint8_t *)channel_ctx->out->payload;
    char payload[] = "Test";
    memcpy(src_payload, payload, sizeof(payload));

    fwk_id_get_element_idx_ExpectAndReturn(
        service_id, FAKE_SERVICE_IDX_OUT_BAND_TEST_CHANNEL);

    status = transport_respond(service_id, NULL, sizeof(payload));

    TEST_ASSERT_EQUAL(FWK_SUCCESS, status);

    uint8_t *dst_payload = (uint8_t *)buffer->payload;
    for (size_t i = 0; i < sizeof(payload); i++) {
        TEST_ASSERT_EQUAL(payload[i], dst_payload[i]);
    }
}

void test_transport_respond_valid_param_ib(void)
{
    int status;

    fwk_id_t service_id = FWK_ID_ELEMENT_INIT(
        FAKE_MODULE_ID, FAKE_SERVICE_IDX_IN_BAND_TEST_CHANNEL);
    char payload[] = "Test";

    struct transport_channel_ctx *channel_ctx =
        &transport_ctx.channel_ctx_table[FAKE_SERVICE_IDX_IN_BAND_TEST_CHANNEL];
    channel_ctx->locked = true;

    fwk_id_get_element_idx_ExpectAndReturn(
        service_id, FAKE_SERVICE_IDX_IN_BAND_TEST_CHANNEL);

    status = transport_respond(service_id, payload, sizeof(payload));

    TEST_ASSERT_EQUAL(FWK_SUCCESS, status);
    TEST_ASSERT_EQUAL(false, channel_ctx->locked);

    uint8_t *dst_payload = (uint8_t *)channel_ctx->out->payload;
    for (size_t i = 0; i < sizeof(payload); i++) {
        TEST_ASSERT_EQUAL(payload[i], dst_payload[i]);
    }
}

void test_transport_respond_valid_param_ob(void)
{
    int status;

    fwk_id_t service_id = FWK_ID_ELEMENT_INIT(
        FAKE_MODULE_ID, FAKE_SERVICE_IDX_OUT_BAND_TEST_CHANNEL);
    char payload[] = "Test";

    struct transport_channel_ctx *channel_ctx =
        &transport_ctx
             .channel_ctx_table[FAKE_SERVICE_IDX_OUT_BAND_TEST_CHANNEL];
    channel_ctx->locked = true;

    struct mod_transport_buffer *buffer = NULL;
    buffer = ((struct mod_transport_buffer *)
                  channel_ctx->config->out_band_mailbox_address);
    buffer->status = MOD_TRANSPORT_MAILBOX_STATUS_FREE_MASK;

    fwk_id_get_element_idx_ExpectAndReturn(
        service_id, FAKE_SERVICE_IDX_OUT_BAND_TEST_CHANNEL);

    status = transport_respond(service_id, payload, sizeof(payload));

    TEST_ASSERT_EQUAL(FWK_SUCCESS, status);
    TEST_ASSERT_EQUAL(false, channel_ctx->locked);

    uint8_t *dst_payload = (uint8_t *)buffer->payload;
    for (size_t i = 0; i < sizeof(payload); i++) {
        TEST_ASSERT_EQUAL(payload[i], dst_payload[i]);
    }
}

void test_transport_transmit_invalid_state_null_buffer_ib(void)
{
    int status;

    fwk_id_t service_id = FWK_ID_ELEMENT_INIT(
        FAKE_MODULE_ID, FAKE_SERVICE_IDX_IN_BAND_TEST_CHANNEL);
    char payload[] = "Test";

    struct transport_channel_ctx *channel_ctx =
        &transport_ctx.channel_ctx_table[FAKE_SERVICE_IDX_IN_BAND_TEST_CHANNEL];
    channel_ctx->locked = true;

    channel_ctx->out = NULL;

    fwk_id_get_element_idx_ExpectAndReturn(
        service_id, FAKE_SERVICE_IDX_IN_BAND_TEST_CHANNEL);

    status = transport_transmit(service_id, 0, payload, sizeof(payload), false);

    TEST_ASSERT_EQUAL(FWK_E_PANIC, status);
}

void test_transport_transmit_invalid_state_null_buffer_ob(void)
{
    int status;

    fwk_id_t service_id = FWK_ID_ELEMENT_INIT(
        FAKE_MODULE_ID, FAKE_SERVICE_IDX_OUT_BAND_TEST_CHANNEL);
    char payload[] = "Test";

    struct transport_channel_ctx *channel_ctx =
        &transport_ctx
             .channel_ctx_table[FAKE_SERVICE_IDX_OUT_BAND_TEST_CHANNEL];
    channel_ctx->locked = true;

    channel_ctx->config->out_band_mailbox_address = (uintptr_t)NULL;

    fwk_id_get_element_idx_ExpectAndReturn(
        service_id, FAKE_SERVICE_IDX_OUT_BAND_TEST_CHANNEL);

    status = transport_transmit(service_id, 0, payload, sizeof(payload), false);

    TEST_ASSERT_EQUAL(FWK_E_PANIC, status);
}

void test_transport_transmit_invalid_param_large_size(void)
{
    int status;

    fwk_id_t service_id = FWK_ID_ELEMENT_INIT(
        FAKE_MODULE_ID, FAKE_SERVICE_IDX_IN_BAND_TEST_CHANNEL);
    char payload[] = "Test";

    struct transport_channel_ctx *channel_ctx =
        &transport_ctx.channel_ctx_table[FAKE_SERVICE_IDX_IN_BAND_TEST_CHANNEL];
    channel_ctx->locked = true;

    fwk_id_get_element_idx_ExpectAndReturn(
        service_id, FAKE_SERVICE_IDX_IN_BAND_TEST_CHANNEL);

    status = transport_transmit(
        service_id, 0, payload, channel_ctx->max_payload_size + 1, false);

    TEST_ASSERT_EQUAL(FWK_E_PARAM, status);
}

void test_transport_transmit_invalid_param_null_payload_ib(void)
{
    int status;

    fwk_id_t service_id = FWK_ID_ELEMENT_INIT(
        FAKE_MODULE_ID, FAKE_SERVICE_IDX_IN_BAND_TEST_CHANNEL);

    struct transport_channel_ctx *channel_ctx =
        &transport_ctx.channel_ctx_table[FAKE_SERVICE_IDX_IN_BAND_TEST_CHANNEL];

    uint8_t *dst_payload = (uint8_t *)channel_ctx->out->payload;
    char payload[] = "Test";
    memcpy(dst_payload, payload, sizeof(payload));

    fwk_id_get_element_idx_ExpectAndReturn(
        service_id, FAKE_SERVICE_IDX_IN_BAND_TEST_CHANNEL);

    status = transport_transmit(service_id, 0, NULL, 5, false);

    TEST_ASSERT_EQUAL(FWK_SUCCESS, status);

    /* Destination payload should be untouched. */
    for (size_t i = 0; i < sizeof(payload); i++) {
        TEST_ASSERT_EQUAL(payload[i], dst_payload[i]);
    }
}

void test_transport_transmit_invalid_param_null_payload_ob(void)
{
    int status;

    fwk_id_t service_id = FWK_ID_ELEMENT_INIT(
        FAKE_MODULE_ID, FAKE_SERVICE_IDX_OUT_BAND_TEST_CHANNEL);

    struct transport_channel_ctx *channel_ctx =
        &transport_ctx
             .channel_ctx_table[FAKE_SERVICE_IDX_OUT_BAND_TEST_CHANNEL];

    struct mod_transport_buffer *buffer = NULL;
    buffer = ((struct mod_transport_buffer *)
                  channel_ctx->config->out_band_mailbox_address);
    buffer->status = MOD_TRANSPORT_MAILBOX_STATUS_FREE_MASK;

    uint8_t *dst_payload = (uint8_t *)buffer->payload;
    char payload[] = "Test";
    memcpy(dst_payload, payload, sizeof(payload));

    fwk_id_get_element_idx_ExpectAndReturn(
        service_id, FAKE_SERVICE_IDX_OUT_BAND_TEST_CHANNEL);

    status = transport_transmit(service_id, 0, NULL, 5, false);

    TEST_ASSERT_EQUAL(FWK_SUCCESS, status);

    /* Destination payload should be untouched. */
    for (size_t i = 0; i < sizeof(payload); i++) {
        TEST_ASSERT_EQUAL(payload[i], dst_payload[i]);
    }
}

void test_transport_transmit_valid_param_ib(void)
{
    int status;

    fwk_id_t channel_id = FWK_ID_ELEMENT_INIT(
        FAKE_MODULE_ID, FAKE_SERVICE_IDX_IN_BAND_TEST_CHANNEL);
    char payload[] = "Test";

    struct transport_channel_ctx *channel_ctx =
        &transport_ctx.channel_ctx_table[FAKE_SERVICE_IDX_IN_BAND_TEST_CHANNEL];
    channel_ctx->locked = true;

    fwk_id_get_element_idx_ExpectAndReturn(
        channel_id, FAKE_SERVICE_IDX_IN_BAND_TEST_CHANNEL);

    status = transport_transmit(channel_id, 0, payload, sizeof(payload), false);

    TEST_ASSERT_EQUAL(FWK_SUCCESS, status);

    uint8_t *dst_payload = (uint8_t *)channel_ctx->out->payload;
    for (size_t i = 0; i < sizeof(payload); i++) {
        TEST_ASSERT_EQUAL(payload[i], dst_payload[i]);
    }
}

void test_transport_transmit_valid_param_ob(void)
{
    int status;

    fwk_id_t channel_id = FWK_ID_ELEMENT_INIT(
        FAKE_MODULE_ID, FAKE_SERVICE_IDX_OUT_BAND_TEST_CHANNEL);
    char payload[] = "Test";

    struct transport_channel_ctx *channel_ctx =
        &transport_ctx
             .channel_ctx_table[FAKE_SERVICE_IDX_OUT_BAND_TEST_CHANNEL];
    channel_ctx->locked = true;

    struct mod_transport_buffer *buffer = NULL;
    buffer = ((struct mod_transport_buffer *)
                  channel_ctx->config->out_band_mailbox_address);
    buffer->status = MOD_TRANSPORT_MAILBOX_STATUS_FREE_MASK;

    fwk_id_get_element_idx_ExpectAndReturn(
        channel_id, FAKE_SERVICE_IDX_OUT_BAND_TEST_CHANNEL);

    status = transport_transmit(channel_id, 0, payload, sizeof(payload), false);

    TEST_ASSERT_EQUAL(FWK_SUCCESS, status);

    uint8_t *dst_payload = (uint8_t *)buffer->payload;
    for (size_t i = 0; i < sizeof(payload); i++) {
        TEST_ASSERT_EQUAL(payload[i], dst_payload[i]);
    }
}

int scmi_test_main(void)
{
    UNITY_BEGIN();

    RUN_TEST(test_transport_payload_size);

    RUN_TEST(test_transport_write_payload_invalid_state_unlocked);
    RUN_TEST(test_transport_write_payload_invalid_param_null_payload);
    RUN_TEST(test_transport_write_payload_invalid_param_large_size);
    RUN_TEST(test_transport_write_payload_invalid_param_large_offset);
    RUN_TEST(test_transport_write_payload_invalid_param_offset_beyond_end);
    RUN_TEST(test_transport_write_payload_valid_param_start);
    RUN_TEST(test_transport_write_payload_valid_param_end);

    RUN_TEST(test_transport_respond_invalid_state_null_buffer_ib);
    RUN_TEST(test_transport_respond_invalid_state_null_buffer_ob);
    RUN_TEST(test_transport_respond_invalid_param_large_size);
    RUN_TEST(test_transport_respond_invalid_param_null_payload_ib);
    RUN_TEST(test_transport_respond_invalid_param_null_payload_ob);
    RUN_TEST(test_transport_respond_valid_param_ib);
    RUN_TEST(test_transport_respond_valid_param_ob);

    RUN_TEST(test_transport_transmit_invalid_state_null_buffer_ib);
    RUN_TEST(test_transport_transmit_invalid_state_null_buffer_ob);
    RUN_TEST(test_transport_transmit_invalid_param_large_size);
    RUN_TEST(test_transport_transmit_invalid_param_null_payload_ib);
    RUN_TEST(test_transport_transmit_invalid_param_null_payload_ob);
    RUN_TEST(test_transport_transmit_valid_param_ib);
    RUN_TEST(test_transport_transmit_valid_param_ob);

    return UNITY_END();
}

#if !defined(TEST_ON_TARGET)
int main(void)
{
    return scmi_test_main();
}
#endif
