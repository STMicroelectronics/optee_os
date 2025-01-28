/*
 * Copyright (c) 2020 Arm Limited. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef PSA_ADAC_MSG_INTERFACE_H
#define PSA_ADAC_MSG_INTERFACE_H

#include <psa_adac.h>
#include <platform/platform.h>
#include <stddef.h>

#if defined(PSA_ADAC_AUTHENTICATOR_IMPLICIT_TRANSPORT)

int msg_interface_init(void *ctx, uint8_t buffer[], size_t size);
int msg_interface_free(void *ctx);

request_packet_t *request_packet_lock(size_t *max_data_size);
response_packet_t *response_packet_lock(size_t *max_data_size);
int response_packet_release(response_packet_t *packet);
int request_packet_release(request_packet_t *packet);

request_packet_t *request_packet_receive();
response_packet_t *response_packet_build(uint16_t status, uint8_t *data, size_t data_size);
int response_packet_send(response_packet_t *packet);

#else

#error "Explicit Transport API Currently not defined"

/* This is a very early draft */

typedef int (*msg_interface_init_t)(void *ctx, uint8_t buffer[], size_t size);
typedef int (*msg_interface_free_t)(void *ctx);

/* Target */
typedef request_packet_t *(*request_packet_receive_t)(void *ctx);
typedef int (*request_packet_release_t)(void *ctx, request_packet_t * packet);
typedef response_packet_t *(*response_packet_lock_t)(void *ctx, size_t *max_data_size);
typedef response_packet_t *(*response_packet_build_t)(void *ctx, uint16_t status, uint8_t *data, size_t data_size);
typedef int (*response_packet_send_t)(void *ctx, response_packet_t *packet);

typedef struct {
    msg_interface_init_t msg_interface_init;
    msg_interface_free_t msg_interface_free;
    request_packet_receive_t request_packet_receive;
    request_packet_release_t request_packet_release;
    response_packet_lock_t response_packet_lock;
    response_packet_build_t response_packet_build;
    response_packet_send_t response_packet_send;
    response_packet_release_t response_packet_release;
} target_msg_interface_t;
#endif

#endif //PSA_ADAC_MSG_INTERFACE_H
