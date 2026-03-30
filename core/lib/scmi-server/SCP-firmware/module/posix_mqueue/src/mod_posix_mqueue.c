/*
 * Arm SCP/MCP Software
 * Copyright (c) 2025, Arm Limited and Contributors. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 * Description:
 *    posix message queue.
 */

#include <mqueue.h>
#include <unistd.h>

#include <mod_posix_mqueue.h>
#include <mod_posix_transport.h>

#include <fwk_assert.h>
#include <fwk_interrupt.h>
#include <fwk_log.h>
#include <fwk_macros.h>
#include <fwk_mm.h>
#include <fwk_module.h>
#include <fwk_module_idx.h>
#include <fwk_status.h>

#include <errno.h>
#include <signal.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>

struct mq_ctx {
    int mqid;
    struct sigevent sev;
    const struct mod_posix_mqueue_queue_config *config;
    const struct mod_posix_transport_driver_input_api *transport_api;
    fwk_id_t listener_id;
    uint8_t *msg_buff;
};

struct mod_posix_mqueue_ctx {
    size_t num_mqueue;
    struct mq_ctx *mq_table_ctx;
};

struct mod_posix_mqueue_ctx mod_posix_mqueue_ctx;

/*
 * Helper functions
 */
struct mq_ctx *get_mq_ctx(fwk_id_t mq_id)
{
    uint32_t mq_idx = fwk_id_get_element_idx(mq_id);
    if (mq_idx >= mod_posix_mqueue_ctx.num_mqueue) {
        fwk_assert(false);
        return NULL;
    }
    return &mod_posix_mqueue_ctx.mq_table_ctx[mq_idx];
}

void read_message(unsigned int channel_idx)
{
    struct mq_ctx *mq_ctx = &mod_posix_mqueue_ctx.mq_table_ctx[channel_idx];
    ssize_t bytes = mq_receive(
        mq_ctx->mqid, mq_ctx->msg_buff, mq_ctx->config->max_msg_size, NULL);
    if (bytes == -1) {
        perror("mq_receive");
    } else {
        FWK_LOG_DEBUG("[MQ] bytes: %ld", bytes);
        FWK_LOG_DEBUG("[MQ] msg: %s", mq_ctx->msg_buff);
    }
}

static int arm_mqueue(struct mq_ctx *mq_ctx)
{
    if (mq_ctx->config->receive) {
        if (mq_notify(mq_ctx->mqid, &mq_ctx->sev) == -1) {
            perror("mq_notify");
            return FWK_E_OS;
        }
    }
    return FWK_SUCCESS;
}

static void posix_mqueue_isr(uintptr_t ctx_ptr)
{
    struct mq_ctx *mq_ctx = (struct mq_ctx *)ctx_ptr;
    FWK_LOG_LOCAL("posix_mqueue_isr");
    struct mq_attr attr;
    if (mq_getattr(mq_ctx->mqid, &attr) == -1) {
        perror("mq_getattr");
    }
    if (attr.mq_curmsgs > 0) {
        mq_ctx->transport_api->signal_message(mq_ctx->listener_id);
        if (mq_getattr(mq_ctx->mqid, &attr) == -1) {
            perror("mq_getattr");
        }
        (void)arm_mqueue(mq_ctx);
    }
}

static int posix_mqueue_get_message(
    struct mod_posix_transport_message *message,
    fwk_id_t device_id)
{
    struct mq_ctx *mq_ctx = get_mq_ctx(device_id);
    ssize_t bytes = mq_receive(
        mq_ctx->mqid, mq_ctx->msg_buff, mq_ctx->config->max_msg_size, NULL);

    if (bytes == -1) {
        perror("mq_receive");
        return FWK_E_DATA;
    } else {
        for (int i = 0; i < 4; ++i) {
            message->message_header <<= 8;
            message->message_header |= mq_ctx->msg_buff[3 - i];
        }
        message->msg_size = bytes;
        memcpy(message->payload, mq_ctx->msg_buff + 4, bytes - 4);
    }
    return FWK_SUCCESS;
}

static int posix_mqueue_send_message(
    struct mod_posix_transport_message *message,
    fwk_id_t device_id)
{
    struct mq_ctx *mq_ctx = get_mq_ctx(device_id);

    memcpy(mq_ctx->msg_buff, (uint8_t *)(&message->message_header), 4);
    memcpy(mq_ctx->msg_buff + 4, message->payload, message->msg_size);
    int status =
        mq_send(mq_ctx->mqid, mq_ctx->msg_buff, message->msg_size + 4, 0);
    if (status != 0) {
        perror("mq_send");
        return FWK_E_DEVICE;
    }

    message->msg_size = 0;
    return FWK_SUCCESS;
}

struct mod_posix_transport_driver_api driver_api = {
    .get_message = posix_mqueue_get_message,
    .send_message = posix_mqueue_send_message,
};

/*
 * Framework API
 */
static int posix_mqueue_init(
    fwk_id_t module_id,
    unsigned int element_count,
    const void *data)
{
    if (element_count == 0) {
        return FWK_E_PARAM;
    }

    mod_posix_mqueue_ctx.num_mqueue = element_count;
    mod_posix_mqueue_ctx.mq_table_ctx =
        fwk_mm_alloc(element_count, sizeof(struct mq_ctx));

    if (mod_posix_mqueue_ctx.mq_table_ctx == NULL) {
        fwk_assert(false);
        return FWK_E_NOMEM;
    }

    return FWK_SUCCESS;
}

static int posix_mqueue_queue_init(
    fwk_id_t mq_id,
    unsigned int notused,
    const void *data)
{
    struct mq_ctx *mq_ctx = get_mq_ctx(mq_id);
    fwk_assert(mq_ctx);

    mq_ctx->config = data;

    struct mq_attr attr = {
        .mq_curmsgs = 0,
        .mq_flags = 0,
        .mq_maxmsg = mq_ctx->config->max_msg_num,
        .mq_msgsize = mq_ctx->config->max_msg_size,
    };

    mq_ctx->msg_buff =
        fwk_mm_alloc(mq_ctx->config->max_msg_size, sizeof(mq_ctx->msg_buff[0]));
    if (mq_ctx->msg_buff == NULL) {
        return FWK_E_NOMEM;
    }

    mq_unlink(mq_ctx->config->mqueue_pathname);
    mqd_t mqd = mq_open(
        mq_ctx->config->mqueue_pathname,
        O_RDWR | O_NONBLOCK | O_CREAT,
        0666,
        &attr);

    if (mqd == (mqd_t)-1) {
        perror("mq_open");
        return FWK_E_INIT;
    }

    mq_ctx->mqid = mqd;
    mq_ctx->sev.sigev_notify = SIGEV_SIGNAL;
    mq_ctx->sev.sigev_signo = mq_ctx->config->posix_signo;
    mq_ctx->sev.sigev_value.sival_int = mq_ctx->config->irq;

    if (mq_ctx->config->receive) {
        if (arm_mqueue(mq_ctx) != FWK_SUCCESS) {
            return FWK_E_INIT;
        }
    }

    return FWK_SUCCESS;
}

static int posix_mqueue_bind(fwk_id_t id, unsigned int round)
{
    int status;

    if (fwk_id_is_type(id, FWK_ID_TYPE_MODULE)) {
        return FWK_SUCCESS;
    }

    if (round == 0) {
    } else if (round == 1 && fwk_id_is_type(id, FWK_ID_TYPE_ELEMENT)) {
        struct mq_ctx *mq_ctx = get_mq_ctx(id);

        status = fwk_module_bind(
            mq_ctx->listener_id,
            FWK_ID_API(
                FWK_MODULE_IDX_POSIX_TRANSPORT,
                MOD_POSIX_TRANSPORT_API_IDX_DRIVER_INPUT),
            &mq_ctx->transport_api);
        if (status != FWK_SUCCESS) {
            return status;
        }
    }

    return FWK_SUCCESS;
}

static int posix_mqueue_process_bind_request(
    fwk_id_t source_id,
    fwk_id_t target_id,
    fwk_id_t api_id,
    const void **api)
{
    /* Only bind to a mqueue (not the whole module) */
    if (!fwk_id_is_type(target_id, FWK_ID_TYPE_ELEMENT)) {
        return FWK_E_ACCESS;
    }
    struct mq_ctx *mq_ctx = get_mq_ctx(target_id);
    switch (fwk_id_get_api_idx(api_id)) {
    case MOD_POSIX_MQUEUE_API_IDX_DRIVER:
        mq_ctx->listener_id = source_id;
        *api = &driver_api;
        break;
    default:
        /* Invalid API */
        fwk_assert(false);
        return FWK_E_PARAM;
    }

    return FWK_SUCCESS;
}

static int posix_mqueue_start(fwk_id_t id)
{
    if (fwk_id_is_type(id, FWK_ID_TYPE_MODULE)) {
        return FWK_SUCCESS;
    }

    struct mq_ctx *mq_ctx = get_mq_ctx(id);

    int status = fwk_interrupt_set_isr_param(
        mq_ctx->config->irq, posix_mqueue_isr, (uintptr_t)mq_ctx);
    if (status != FWK_SUCCESS) {
        return status;
    }

    return fwk_interrupt_enable(mq_ctx->config->irq);

    return FWK_SUCCESS;
}

const struct fwk_module module_posix_mqueue = {
    .type = FWK_MODULE_TYPE_SERVICE,
    .api_count = MOD_POSIX_MQUEUE_API_IDX_COUNT,
    .init = posix_mqueue_init,
    .element_init = posix_mqueue_queue_init,
    .bind = posix_mqueue_bind,
    .start = posix_mqueue_start,
    .process_bind_request = posix_mqueue_process_bind_request,
};
