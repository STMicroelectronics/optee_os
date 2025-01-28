/*
 * Copyright (c) 2020-2023 Arm Limited. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include "psa_adac_config.h"
#include "psa_adac.h"
#include "psa_adac_debug.h"
#include "platform/platform.h"
#include "platform/msg_interface.h"
#include "static_buffer_msg.h"
#include "microsecond_timer.h"
#include <stddef.h>
#include <stdio.h>

#ifndef PSA_ADAC_AUTHENTICATOR_IMPLICIT_TRANSPORT
#error "Unsupported environment"
#endif

#if !(defined(SDM_MEMORY_WINDOW_BASE) && defined(SDM_MEMORY_WINDOW_SIZE))
#error "Memory window not configured"
#endif

/*
 gdbserver start
 reset -h
 w32 0x30000000 0xFF00FF00
 w32 0x30000004 0x00FF00FF
 w32 0x30000008 0xFF00FF00
 w32 0x3000000C 0x00FF00FF
 */

enum {
    HOST_DONE = 0x12121212,
    TARGET_DONE = 0xEFEFEFEF
};

static volatile uint32_t *const sdm_memory_window_base = (uint32_t *) (SDM_MEMORY_WINDOW_BASE);

int psa_adac_detect_debug_request(void)
{
    PSA_ADAC_LOG_DEBUG("memw", "Waiting for debug request\r\n");
    while (!(sdm_memory_window_base[0] == 0xFF00FF00 &&
            sdm_memory_window_base[1] == 0x00FF00FF &&
            sdm_memory_window_base[2] == 0xFF00FF00 &&
            sdm_memory_window_base[3] == 0x00FF00FF)) {
        sleep_ms(100);
    };
    return 1;
}

void psa_adac_acknowledge_debug_request(void)
{
    PSA_ADAC_LOG_DEBUG("memw", "Acknowledging request\r\n");
    sdm_memory_window_base[0] = 0x00FF00FF;
    sdm_memory_window_base[1] = 0xFF00FF00;
    sdm_memory_window_base[2] = 0x00FF00FF;
    sdm_memory_window_base[3] = 0xFF00FF00;
}

int msg_interface_init(void *ctx, uint8_t buffer[], size_t buffer_size)
{
    if (buffer_size > SDM_MEMORY_WINDOW_SIZE - 16) {
        return -1;
    }
    sdm_memory_window_base[1] = buffer_size;
    return psa_adac_static_buffer_msg_init(
                            (uint8_t *) (SDM_MEMORY_WINDOW_BASE + 16), SDM_MEMORY_WINDOW_SIZE - 16);
}

int msg_interface_free(void *ctx)
{
    return psa_adac_static_buffer_msg_release();
}


int request_packet_send(request_packet_t *packet)
{
    if (packet != NULL) {
        /* TODO: Copy */
    }
    sdm_memory_window_base[0] = HOST_DONE;
    return 0;
}

request_packet_t *request_packet_receive(response_packet_t *packet)
{
    size_t max = 0;
    request_packet_t *r = request_packet_lock(&max);

    while (sdm_memory_window_base[0] != HOST_DONE) {
        /* Loop */
    }
    /* TODO: Check size consistency (max vs r->data_count * 4) */
    return r;
}

int response_packet_send(response_packet_t *packet)
{
    if (packet != NULL) {
        /* TODO: Copy */
    }
    sdm_memory_window_base[0] = TARGET_DONE;
    return 0;
}

response_packet_t *response_packet_receive(void)
{
    size_t max = 0;
    response_packet_t *r = response_packet_lock(&max);
    while (sdm_memory_window_base[0] != TARGET_DONE) {
        /* Loop */
    }
    /* TODO: Check size consistency (max vs r->data_count * 4) */
    return r;
}
