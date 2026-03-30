/*
 * Arm SCP/MCP Software
 * Copyright (c) 2025, Arm Limited and Contributors. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 * Description:
 *    posix transport.
 */

#include <mod_posix_transport.h>
#include <mod_scmi.h>

#include <fwk_assert.h>
#include <fwk_interrupt.h>
#include <fwk_log.h>
#include <fwk_macros.h>
#include <fwk_mm.h>
#include <fwk_module.h>
#include <fwk_module_idx.h>
#include <fwk_status.h>

#include <stdbool.h>
#include <string.h>

struct channel_ctx {
    /* Channel identifier */
    fwk_id_t id;
    /* Transmitting Device APIs */
    struct mod_posix_transport_driver_api *tx_driver_api;
    /* Receiving Device APIs */
    struct mod_posix_transport_driver_api *rx_driver_api;
    /* Channel configuration data */
    struct mod_posix_transport_channel_config *config;
    /* Service bound to the channel */
    fwk_id_t listener_id;
    /* Service bound to the channel API */
    struct mod_scmi_from_transport_api *listener_signal_api;
    /* MSG received */
    struct mod_posix_transport_message in_message;
    /* MSG to be transmite or (transmitted) */
    struct mod_posix_transport_message out_message;
};

struct posix_transport_ctx {
    /* Table of channel contexts */
    struct channel_ctx *channel_ctx_table;
    /* Number of channels */
    unsigned int channel_count;
};

static struct posix_transport_ctx posix_transport_ctx;

static inline bool is_channel_idx_valid(size_t idx)
{
    return idx < posix_transport_ctx.channel_count;
}

static inline bool is_channel_valid(fwk_id_t id)
{
    size_t elt_idx = fwk_id_get_element_idx(id);
    return is_channel_idx_valid(elt_idx);
}

static struct channel_ctx *get_channel_ctx(fwk_id_t id)
{
    size_t elt_idx = fwk_id_get_element_idx(id);
    return is_channel_idx_valid(elt_idx) ?
        &posix_transport_ctx.channel_ctx_table[elt_idx] :
        NULL;
}

/*
 * SCMI module Transport API
 */
static int posix_transport_get_secure(fwk_id_t channel_id, bool *secure)
{
    if (secure == NULL) {
        return FWK_E_PARAM;
    }

    /* Posix transport has no support for secure channels yet.*/
    *secure = false;
    return FWK_SUCCESS;
}

static int posix_transport_get_max_payload_size(
    fwk_id_t channel_id,
    size_t *size)
{
    struct channel_ctx *channel_ctx;

    if (size == NULL) {
        return FWK_E_PARAM;
    }

    channel_ctx = get_channel_ctx(channel_id);
    if (channel_ctx == NULL) {
        return FWK_E_PARAM;
    }

    *size = sizeof(channel_ctx->out_message.payload);
    return FWK_SUCCESS;
}

static int posix_transport_get_message_header(
    fwk_id_t channel_id,
    uint32_t *header)
{
    struct channel_ctx *channel_ctx;

    if (header == NULL) {
        return FWK_E_PARAM;
    }

    channel_ctx = get_channel_ctx(channel_id);
    if (channel_ctx == NULL) {
        return FWK_E_PARAM;
    }

    *header = channel_ctx->in_message.message_header;
    return FWK_SUCCESS;
}

static int posix_transport_get_payload(
    fwk_id_t channel_id,
    const void **payload,
    size_t *size)
{
    struct channel_ctx *channel_ctx;

    if (payload == NULL || size == NULL) {
        return FWK_E_PARAM;
    }

    channel_ctx = get_channel_ctx(channel_id);
    if (channel_ctx == NULL) {
        return FWK_E_PARAM;
    }

    *payload = &channel_ctx->in_message.payload;
    *size = channel_ctx->in_message.msg_size - 4;
    return FWK_SUCCESS;
}

static int posix_transport_write_payload(
    fwk_id_t channel_id,
    size_t offset,
    const void *payload,
    size_t size)
{
    struct channel_ctx *channel_ctx;

    if (payload == NULL) {
        return FWK_E_PARAM;
    }

    channel_ctx = get_channel_ctx(channel_id);
    if (channel_ctx == NULL) {
        return FWK_E_PARAM;
    }

    if (offset + size > sizeof(channel_ctx->out_message.payload)) {
        return FWK_E_DATA;
    }

    channel_ctx->out_message.msg_size += size;
    memcpy(channel_ctx->out_message.payload + offset, payload, size);

    return FWK_SUCCESS;
}

static int posix_transport_transmit(
    fwk_id_t channel_id,
    uint32_t message_header,
    const void *payload,
    size_t size,
    bool request_ack_by_interrupt)
{
    return FWK_E_SUPPORT;
}

static int posix_transport_respond(
    fwk_id_t channel_id,
    const void *payload,
    size_t size)
{
    int status;
    struct channel_ctx *channel_ctx = get_channel_ctx(channel_id);
    if (channel_ctx == NULL) {
        return FWK_E_PARAM;
    }

    if (payload) {
        struct mod_posix_transport_message resp = {
            .msg_size = size,
            .message_header = channel_ctx->in_message.message_header
        };
        memcpy(resp.payload, payload, size);
        status = channel_ctx->tx_driver_api->send_message(
            &resp, channel_ctx->config->tx_dev.dev_driver_id);
    } else {
        channel_ctx->out_message.message_header =
            channel_ctx->in_message.message_header;
        status = channel_ctx->tx_driver_api->send_message(
            &channel_ctx->out_message,
            channel_ctx->config->tx_dev.dev_driver_id);
    }
    return status;
}

static const struct mod_scmi_to_transport_api scmi_to_transport_api = {
    .get_secure = posix_transport_get_secure,
    .get_max_payload_size = posix_transport_get_max_payload_size,
    .get_message_header = posix_transport_get_message_header,
    .get_payload = posix_transport_get_payload,
    .write_payload = posix_transport_write_payload,
    .respond = posix_transport_respond,
    .transmit = posix_transport_transmit,
};

/*
 * Driver handler API
 */
static int posix_transport_signal_message(fwk_id_t channel_id)
{
    int status;
    struct channel_ctx *channel_ctx = get_channel_ctx(channel_id);
    if (channel_ctx == NULL) {
        return FWK_E_PARAM;
    }

    status = channel_ctx->rx_driver_api->get_message(
        &channel_ctx->in_message, channel_ctx->config->rx_dev.dev_driver_id);
    if (status != FWK_SUCCESS) {
        return status;
    }

    return channel_ctx->listener_signal_api->signal_message(
        channel_ctx->listener_id);
}

static const struct mod_posix_transport_driver_input_api driver_input_api = {
    .signal_message = posix_transport_signal_message,
};

/*
 * Framework API
 */
static int posix_transport_init(
    fwk_id_t module_id,
    unsigned int element_count,
    const void *data)
{
    if (element_count == 0) {
        return FWK_E_PARAM;
    }

    posix_transport_ctx.channel_ctx_table = fwk_mm_calloc(
        element_count, sizeof(*posix_transport_ctx.channel_ctx_table));
    if (posix_transport_ctx.channel_ctx_table == NULL) {
        return FWK_E_NOMEM;
    }

    posix_transport_ctx.channel_count = element_count;

    return FWK_SUCCESS;
}

static int posix_transport_channel_init(
    fwk_id_t channel_id,
    unsigned int slot_count,
    const void *data)
{
    size_t elt_idx = fwk_id_get_element_idx(channel_id);
    struct channel_ctx *channel_ctx =
        &posix_transport_ctx.channel_ctx_table[elt_idx];

    channel_ctx->config = (struct mod_posix_transport_channel_config *)data;

    return FWK_SUCCESS;
}

static int posix_transport_bind(fwk_id_t id, unsigned int round)
{
    int status;
    struct channel_ctx *channel_ctx;

    if (fwk_id_is_type(id, FWK_ID_TYPE_MODULE)) {
        return FWK_SUCCESS;
    }

    channel_ctx =
        &posix_transport_ctx.channel_ctx_table[fwk_id_get_element_idx(id)];

    if (round == 0) {
        status = fwk_module_bind(
            channel_ctx->config->tx_dev.dev_driver_id,
            channel_ctx->config->tx_dev.dev_driver_api_id,
            &channel_ctx->tx_driver_api);
        if (status != FWK_SUCCESS) {
            return status;
        }

        status = fwk_module_bind(
            channel_ctx->config->rx_dev.dev_driver_id,
            channel_ctx->config->rx_dev.dev_driver_api_id,
            &channel_ctx->rx_driver_api);
        if (status != FWK_SUCCESS) {
            return status;
        }

    } else if (round == 1) {
        status = fwk_module_bind(
            channel_ctx->listener_id,
            FWK_ID_API(FWK_MODULE_IDX_SCMI, MOD_SCMI_API_IDX_TRANSPORT),
            &channel_ctx->listener_signal_api);
        if (status != FWK_SUCCESS) {
            return status;
        }
    }

    return FWK_SUCCESS;
}

static int posix_transport_process_bind_request(
    fwk_id_t source_id,
    fwk_id_t target_id,
    fwk_id_t api_id,
    const void **api)
{
    struct channel_ctx *channel_ctx = NULL;

    /* Only bind to a channel */
    if (!fwk_id_is_type(target_id, FWK_ID_TYPE_ELEMENT)) {
        return FWK_E_ACCESS;
    }

    channel_ctx = get_channel_ctx(target_id);
    if (channel_ctx == NULL) {
        return FWK_E_PARAM;
    }

    switch (fwk_id_get_api_idx(api_id)) {
    case MOD_POSIX_TRANSPORT_API_IDX_DRIVER_INPUT:
        if (fwk_id_is_equal(
                source_id, channel_ctx->config->rx_dev.dev_driver_id) ||
            fwk_id_is_equal(
                source_id, channel_ctx->config->tx_dev.dev_driver_id)) {
            *api = &driver_input_api;
        } else {
            return FWK_E_ACCESS;
        }
        break;
    case MOD_POSIX_TRANSPORT_API_IDX_SCMI_TRANSPORT:
        *api = &scmi_to_transport_api;
        channel_ctx->listener_id = source_id;
        break;
    default:
        return FWK_E_PARAM;
    }

    return FWK_SUCCESS;
}

static int posix_transport_start(fwk_id_t id)
{
    return FWK_SUCCESS;
}

const struct fwk_module module_posix_transport = {
    .type = FWK_MODULE_TYPE_SERVICE,
    .api_count = MOD_POSIX_TRANSPORT_API_IDX_COUNT,
    .init = posix_transport_init,
    .element_init = posix_transport_channel_init,
    .bind = posix_transport_bind,
    .start = posix_transport_start,
    .process_bind_request = posix_transport_process_bind_request,
};
