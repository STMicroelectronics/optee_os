/*
 * Arm SCP/MCP Software
 * Copyright (c) 2025, Arm Limited and Contributors. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include "scp_unity.h"
#include "unity.h"

#ifdef TEST_ON_TARGET
#    include <fwk_module.h>
#else
#    include <Mockfwk_module.h>
#endif

#include <Mockmod_posix_transport_extra.h>

#include <mod_posix_transport.h>

#include <fwk_element.h>
#include <fwk_id.h>
#include <fwk_macros.h>

#include UNIT_TEST_SRC

struct mod_posix_transport_driver_api dev_api = {
    .get_message = posix_dev_get_message,
    .send_message = posix_dev_send_message,
};

struct mod_scmi_from_transport_api listener_api = {
    .signal_message = listener_signal_message,
};

static struct mod_posix_transport_channel_config channel_config[2] = {
    [0] = { .tx_dev = { .dev_driver_id =
                            FWK_ID_ELEMENT(FWK_MODULE_IDX_POSIX_DEVICE, 0) },
            .rx_dev = { .dev_driver_id =
                            FWK_ID_ELEMENT(FWK_MODULE_IDX_POSIX_DEVICE, 1) } },

    [1] = { .tx_dev = { .dev_driver_id =
                            FWK_ID_ELEMENT(FWK_MODULE_IDX_POSIX_DEVICE, 2) },
            .rx_dev = { .dev_driver_id =
                            FWK_ID_ELEMENT(FWK_MODULE_IDX_POSIX_DEVICE, 3) } },

};

#define CHANNEL_INIT( \
    IDX, TX_DEV_APIS, RX_DEV_APIS, CONFIG, LISTENER_MOD_IDX, LISTENER_APIS) \
    { \
        .id = FWK_ID_ELEMENT_INIT(FWK_MODULE_IDX_POSIX_TRANSPORT, IDX), \
        .tx_driver_api = TX_DEV_APIS, .rx_driver_api = RX_DEV_APIS, \
        .config = CONFIG, .listener_id = FWK_ID_MODULE(LISTENER_MOD_IDX), \
        .listener_signal_api = LISTENER_APIS, \
    }

struct channel_ctx test_channel_ctx[2] = {
    [0] = CHANNEL_INIT(
        0,
        &dev_api,
        &dev_api,
        &channel_config[0],
        FWK_MODULE_IDX_SCMI,
        &listener_api),
    [1] = CHANNEL_INIT(
        1,
        &dev_api,
        &dev_api,
        &channel_config[1],
        FWK_MODULE_IDX_SCMI,
        &listener_api),
};

static void fill_in_message(
    unsigned idx,
    size_t payload_len,
    uint32_t header,
    uint8_t pattern_base)
{
    struct channel_ctx *channel_ctx = &test_channel_ctx[idx];

    memset(&channel_ctx->in_message, 0, sizeof(channel_ctx->in_message));
    channel_ctx->in_message.message_header = header;
    channel_ctx->in_message.msg_size = 4 + payload_len;

    for (size_t i = 0; i < payload_len; ++i) {
        channel_ctx->in_message.payload[i] =
            (uint8_t)(pattern_base + (uint8_t)i);
    }
}

void setUp(void)
{
    Mockmod_posix_transport_extra_Init();

    posix_transport_ctx.channel_ctx_table = test_channel_ctx;
    posix_transport_ctx.channel_count = FWK_ARRAY_SIZE(test_channel_ctx);
    test_channel_ctx[0].tx_driver_api = &dev_api;
    test_channel_ctx[1].tx_driver_api = &dev_api;

    for (unsigned int i = 0; i < posix_transport_ctx.channel_count; ++i) {
        memset(
            &test_channel_ctx[i].out_message,
            0,
            sizeof(test_channel_ctx[i].out_message));
    }
}

void tearDown(void)
{
    Mockmod_posix_transport_extra_Verify();
}

void test_posix_transport_get_max_payload_size(void)
{
    fwk_id_t channel_id = FWK_ID_ELEMENT(FWK_MODULE_IDX_POSIX_TRANSPORT, 0);
    size_t max_payload_size = 0;

    int status =
        posix_transport_get_max_payload_size(channel_id, &max_payload_size);

    TEST_ASSERT_EQUAL(FWK_SUCCESS, status);
    TEST_ASSERT_EQUAL(POSIX_TRANSPORT_MSG_MAX_SIZE, max_payload_size);
}

void test_posix_transport_payload_size_invalid_params(void)
{
    fwk_id_t channel_id = FWK_ID_ELEMENT(FWK_MODULE_IDX_POSIX_TRANSPORT, 0);
    size_t max_payload_size = 0;
    int status = posix_transport_get_max_payload_size(channel_id, NULL);

    TEST_ASSERT_EQUAL(FWK_E_PARAM, status);
    TEST_ASSERT_EQUAL(0, max_payload_size);

    channel_id = FWK_ID_ELEMENT(
        FWK_MODULE_IDX_POSIX_TRANSPORT, posix_transport_ctx.channel_count);
    status =
        posix_transport_get_max_payload_size(channel_id, &max_payload_size);

    TEST_ASSERT_EQUAL(FWK_E_PARAM, status);
    TEST_ASSERT_EQUAL(0, max_payload_size);
}

void test_posix_transport_write_payload_null_payload(void)
{
    unsigned int channel_idx = 0;
    fwk_id_t channel_id =
        FWK_ID_ELEMENT(FWK_MODULE_IDX_POSIX_TRANSPORT, channel_idx);

    int status = posix_transport_write_payload(channel_id, 0, NULL, 8);

    TEST_ASSERT_EQUAL(FWK_E_PARAM, status);
    TEST_ASSERT_EQUAL(0, test_channel_ctx[0].out_message.msg_size);
}

void test_posix_transport_write_payload_invalid_channel_id(void)
{
    fwk_id_t bad_id = FWK_ID_ELEMENT(
        FWK_MODULE_IDX_POSIX_TRANSPORT, posix_transport_ctx.channel_count);

    uint8_t buf[4] = { 0 };
    int status = posix_transport_write_payload(bad_id, 0, buf, sizeof(buf));

    TEST_ASSERT_EQUAL(FWK_E_PARAM, status);
}

void test_posix_transport_write_payload_oversize(void)
{
    fwk_id_t channel_id = FWK_ID_ELEMENT(FWK_MODULE_IDX_POSIX_TRANSPORT, 0);
    static uint8_t big_buff[POSIX_TRANSPORT_MSG_MAX_SIZE + 1];

    int status = posix_transport_write_payload(
        channel_id, 0, big_buff, POSIX_TRANSPORT_MSG_MAX_SIZE + 1);

    TEST_ASSERT_EQUAL(FWK_E_DATA, status);
    TEST_ASSERT_EQUAL(0, test_channel_ctx[0].out_message.msg_size);
}

void test_posix_transport_write_payload_offset_too_large(void)
{
    fwk_id_t channel_id = FWK_ID_ELEMENT(FWK_MODULE_IDX_POSIX_TRANSPORT, 0);
    uint8_t byte = 0xAA;
    size_t offset = POSIX_TRANSPORT_MSG_MAX_SIZE;

    int status = posix_transport_write_payload(channel_id, offset, &byte, 1);

    TEST_ASSERT_EQUAL(FWK_E_DATA, status);
    TEST_ASSERT_EQUAL(0, test_channel_ctx[0].out_message.msg_size);
}

void test_posix_transport_write_payload_offset_plus_size_beyond_end(void)
{
    fwk_id_t channel_id = FWK_ID_ELEMENT(FWK_MODULE_IDX_POSIX_TRANSPORT, 0);
    size_t offset = POSIX_TRANSPORT_MSG_MAX_SIZE - 1;
    uint8_t buff[2] = { 0x11, 0x22 };

    int status =
        posix_transport_write_payload(channel_id, offset, buff, sizeof(buff));

    TEST_ASSERT_EQUAL(FWK_E_DATA, status);
    TEST_ASSERT_EQUAL(0, test_channel_ctx[0].out_message.msg_size);
}

void test_posix_transport_write_payload_success_small_at_zero(void)
{
    fwk_id_t channel_id = FWK_ID_ELEMENT(FWK_MODULE_IDX_POSIX_TRANSPORT, 0);
    const char payload[] = "ABCDEFGH";

    int status =
        posix_transport_write_payload(channel_id, 0, payload, sizeof(payload));

    TEST_ASSERT_EQUAL(FWK_SUCCESS, status);
    TEST_ASSERT_EQUAL(
        sizeof(payload), test_channel_ctx[0].out_message.msg_size);
    TEST_ASSERT_EQUAL_UINT8_ARRAY(
        payload, test_channel_ctx[0].out_message.payload, sizeof(payload));
}

void test_posix_transport_write_payload_success_two_contiguous_chunks(void)
{
    fwk_id_t channel_id = FWK_ID_ELEMENT(FWK_MODULE_IDX_POSIX_TRANSPORT, 1);

    const uint8_t p0[] = { 1, 2, 3, 4, 5, 6, 7, 8 };
    const uint8_t p1[] = { 9, 10, 11, 12 };
    const uint8_t expected[] = { 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12 };

    int status = posix_transport_write_payload(channel_id, 0, p0, sizeof(p0));
    TEST_ASSERT_EQUAL(FWK_SUCCESS, status);

    status =
        posix_transport_write_payload(channel_id, sizeof(p0), p1, sizeof(p1));
    TEST_ASSERT_EQUAL(FWK_SUCCESS, status);

    TEST_ASSERT_EQUAL(
        sizeof(expected), test_channel_ctx[1].out_message.msg_size);
    TEST_ASSERT_EQUAL_UINT8_ARRAY(
        expected, test_channel_ctx[1].out_message.payload, sizeof(expected));
}

void test_posix_transport_write_payload_success_exact_fit(void)
{
    fwk_id_t channel_id = FWK_ID_ELEMENT(FWK_MODULE_IDX_POSIX_TRANSPORT, 0);
    size_t max_size = POSIX_TRANSPORT_MSG_MAX_SIZE;
    static uint8_t fill[POSIX_TRANSPORT_MSG_MAX_SIZE];

    for (size_t i = 0; i < max_size; ++i) {
        fill[i] = (uint8_t)(i & 0xFF);
    }

    int status = posix_transport_write_payload(channel_id, 0, fill, max_size);

    TEST_ASSERT_EQUAL(FWK_SUCCESS, status);
    TEST_ASSERT_EQUAL(max_size, test_channel_ctx[0].out_message.msg_size);
    TEST_ASSERT_EQUAL_UINT8_ARRAY(
        fill, test_channel_ctx[0].out_message.payload, max_size);
}

void test_posix_transport_get_payload_success(void)
{
    const unsigned ch_idx = 0;
    fwk_id_t channel_id =
        FWK_ID_ELEMENT(FWK_MODULE_IDX_POSIX_TRANSPORT, ch_idx);
    const size_t len = 16;
    const void *payload = NULL;
    size_t size = 0;
    uint8_t pattern_base = 0x1;

    fill_in_message(ch_idx, len, /*header=*/0xAABBCCDDu, pattern_base);

    int status = posix_transport_get_payload(channel_id, &payload, &size);

    TEST_ASSERT_EQUAL(FWK_SUCCESS, status);
    TEST_ASSERT_NOT_NULL(payload);
    TEST_ASSERT_EQUAL(len, size);

    TEST_ASSERT_EQUAL_PTR(
        &test_channel_ctx[ch_idx].in_message.payload, payload);

    const uint8_t *u8 = (const uint8_t *)payload;
    for (size_t i = 0; i < len; ++i) {
        TEST_ASSERT_EQUAL_UINT8((uint8_t)(pattern_base + i), u8[i]);
    }
}

void test_posix_transport_get_payload_success_zero_length(void)
{
    const unsigned ch_idx = 1;
    fwk_id_t channel_id =
        FWK_ID_ELEMENT(FWK_MODULE_IDX_POSIX_TRANSPORT, ch_idx);
    const size_t len = 0;
    const void *payload = (const void *)0xDEADBEEF;
    size_t size = 123;

    fill_in_message(ch_idx, len, /*header=*/0x12345678u, /*pattern_base=*/0);

    int status = posix_transport_get_payload(channel_id, &payload, &size);

    TEST_ASSERT_EQUAL(FWK_SUCCESS, status);
    TEST_ASSERT_NOT_NULL(payload);
    TEST_ASSERT_EQUAL(0, size);
    TEST_ASSERT_EQUAL_PTR(
        &test_channel_ctx[ch_idx].in_message.payload, payload);
}

void test_posix_transport_get_payload_invalid_params(void)
{
    fwk_id_t channel_id = FWK_ID_ELEMENT(FWK_MODULE_IDX_POSIX_TRANSPORT, 0);
    int status = FWK_SUCCESS;
    const void *payload = NULL;
    size_t size = 0;

    status = posix_transport_get_payload(channel_id, NULL, &size);
    TEST_ASSERT_EQUAL(FWK_E_PARAM, status);

    status = posix_transport_get_payload(channel_id, &payload, NULL);
    TEST_ASSERT_EQUAL(FWK_E_PARAM, status);
}

void test_posix_transport_get_payload_invalid_channel_id(void)
{
    fwk_id_t channel_id = FWK_ID_ELEMENT(
        FWK_MODULE_IDX_POSIX_TRANSPORT, posix_transport_ctx.channel_count);
    const void *payload = NULL;
    size_t size = 0;

    int status = posix_transport_get_payload(channel_id, &payload, &size);
    TEST_ASSERT_EQUAL(FWK_E_PARAM, status);
}

void test_posix_transport_respond_invalid_channel_id(void)
{
    fwk_id_t bad_id = FWK_ID_ELEMENT(
        FWK_MODULE_IDX_POSIX_TRANSPORT, posix_transport_ctx.channel_count);

    /* If the function accidentally calls send_message,
     *  the mock will flag an unexpected call in tearDown().
     */
    int status = posix_transport_respond(bad_id, /*payload=*/NULL, /*size=*/0);

    TEST_ASSERT_EQUAL(FWK_E_PARAM, status);
}

void test_posix_transport_respond_with_payload_success(void)
{
    const unsigned ch_idx = 0;
    fwk_id_t channel_id =
        FWK_ID_ELEMENT(FWK_MODULE_IDX_POSIX_TRANSPORT, ch_idx);
    const uint8_t resp_payload[] = { 0xDE, 0xAD, 0xBE, 0xEF };

    fill_in_message(ch_idx, /*payload_len=*/5, /*header=*/0xDEADCAFEu, 0x10);

    struct mod_posix_transport_message resp = {
        .msg_size = FWK_ARRAY_SIZE(resp_payload),
        .message_header = test_channel_ctx[ch_idx].in_message.message_header,
    };
    memcpy(resp.payload, resp_payload, FWK_ARRAY_SIZE(resp_payload));

    /* Expect exactly one send_message call (args validated loosely here).
       If it's not called, or called more than once, the mock will fail. */
    posix_dev_send_message_ExpectAndReturn(
        &resp,
        test_channel_ctx[ch_idx].config->tx_dev.dev_driver_id,
        FWK_SUCCESS);

    size_t pre_respond_msg_size = test_channel_ctx[ch_idx].out_message.msg_size;

    int status = posix_transport_respond(
        channel_id, resp_payload, FWK_ARRAY_SIZE(resp_payload));

    TEST_ASSERT_EQUAL(FWK_SUCCESS, status);

    /* Ensure the preallocated out_message buffer wasn't modified by this path
     */
    TEST_ASSERT_EQUAL(
        pre_respond_msg_size, test_channel_ctx[ch_idx].out_message.msg_size);
}

void test_posix_transport_respond_without_payload_uses_out_message(void)
{
    const unsigned ch_idx = 1;
    fwk_id_t channel_id =
        FWK_ID_ELEMENT(FWK_MODULE_IDX_POSIX_TRANSPORT, ch_idx);
    const uint32_t inbound_header = 0xAABBCCDDu;
    const uint8_t payload[] = { 1, 2, 3, 4, 5 };

    fill_in_message(ch_idx, /*payload_len=*/3, /*header=*/inbound_header, 0x5A);

    memcpy(
        test_channel_ctx[ch_idx].out_message.payload,
        payload,
        FWK_ARRAY_SIZE(payload));

    test_channel_ctx[ch_idx].out_message.msg_size = FWK_ARRAY_SIZE(payload);
    test_channel_ctx[ch_idx].out_message.message_header = 0x0;

    posix_dev_send_message_ExpectAndReturn(
        &test_channel_ctx[ch_idx].out_message,
        test_channel_ctx[ch_idx].config->tx_dev.dev_driver_id,
        FWK_SUCCESS);

    int status = posix_transport_respond(channel_id, NULL, 0);

    TEST_ASSERT_EQUAL(FWK_SUCCESS, status);

    TEST_ASSERT_EQUAL(
        inbound_header, test_channel_ctx[ch_idx].out_message.message_header);

    TEST_ASSERT_EQUAL(
        FWK_ARRAY_SIZE(payload), test_channel_ctx[ch_idx].out_message.msg_size);

    TEST_ASSERT_EQUAL_UINT8_ARRAY(
        payload,
        test_channel_ctx[ch_idx].out_message.payload,
        FWK_ARRAY_SIZE(payload));
}
int posix_transport_test_main(void)
{
    UNITY_BEGIN();

    RUN_TEST(test_posix_transport_get_max_payload_size);

    RUN_TEST(test_posix_transport_write_payload_null_payload);
    RUN_TEST(test_posix_transport_write_payload_invalid_channel_id);
    RUN_TEST(test_posix_transport_write_payload_oversize);
    RUN_TEST(test_posix_transport_write_payload_offset_too_large);
    RUN_TEST(test_posix_transport_write_payload_offset_plus_size_beyond_end);
    RUN_TEST(test_posix_transport_write_payload_success_small_at_zero);
    RUN_TEST(test_posix_transport_write_payload_success_two_contiguous_chunks);
    RUN_TEST(test_posix_transport_write_payload_success_exact_fit);

    RUN_TEST(test_posix_transport_get_payload_success);
    RUN_TEST(test_posix_transport_get_payload_success_zero_length);
    RUN_TEST(test_posix_transport_get_payload_invalid_params);
    RUN_TEST(test_posix_transport_get_payload_invalid_channel_id);

    RUN_TEST(test_posix_transport_respond_invalid_channel_id);
    RUN_TEST(test_posix_transport_respond_with_payload_success);
    RUN_TEST(test_posix_transport_respond_without_payload_uses_out_message);

    return UNITY_END();
}

#if !defined(TEST_ON_TARGET)
int main(void)
{
    return posix_transport_test_main();
}
#endif
