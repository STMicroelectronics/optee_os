/*
 * Copyright (c) 2020 Arm Limited. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include "static_buffer_msg.h"

#include <string.h>

// TODO: Enforce alignment and sizes on 4 bytes

enum {
    BUFFER_UNINITIALIZED = 0,
    BUFFER_EMPTY,
    BUFFER_REQUEST,
    BUFFER_RESPONSE
};

static size_t psa_adac_static_buffer_size = 0;
static uint8_t *psa_adac_static_buffer_pointer = NULL;
static uint8_t psa_adac_static_buffer_status = BUFFER_UNINITIALIZED;

int psa_adac_static_buffer_msg_init(uint8_t *buffer, size_t size) {
    int ret = -1;
    if (psa_adac_static_buffer_status == BUFFER_UNINITIALIZED) {
        psa_adac_static_buffer_size = size;
        psa_adac_static_buffer_pointer = buffer;
        psa_adac_static_buffer_status = BUFFER_EMPTY;
        ret = 0;
    }

    return ret;
}

int psa_adac_static_buffer_msg_release() {
    int ret = -1;
    if (psa_adac_static_buffer_status == BUFFER_EMPTY) {
        psa_adac_static_buffer_size = 0;
        psa_adac_static_buffer_pointer = NULL;
        psa_adac_static_buffer_status = BUFFER_UNINITIALIZED;
        ret = 0;
    }

    return ret;
}

request_packet_t *psa_adac_static_buffer_msg_get_request() {
    // TODO: Check consistency
    return (request_packet_t *) psa_adac_static_buffer_pointer;
}

response_packet_t *psa_adac_static_buffer_msg_get_response() {
    // TODO: Check consistency
    return (response_packet_t *) psa_adac_static_buffer_pointer;

}

request_packet_t *request_packet_build(uint16_t command, uint8_t *data, size_t data_size) {
    request_packet_t *request = NULL;
    if ((psa_adac_static_buffer_status == BUFFER_EMPTY) &&
        (data_size <= (psa_adac_static_buffer_size - sizeof(request_packet_t)))) {
        request = (request_packet_t *) psa_adac_static_buffer_pointer;
        request->command = command;
        request->data_count = data_size / 4UL;
        (void) memcpy((void *) request->data, (void *) data, data_size);
        // TODO: Fill with 0s
        psa_adac_static_buffer_status = BUFFER_REQUEST;
    }

    return request;
}

request_packet_t *request_packet_lock(size_t *max_data_size) {
    request_packet_t *request = NULL;
    if (psa_adac_static_buffer_status == BUFFER_EMPTY) {
        if (max_data_size != NULL) {
            *max_data_size = psa_adac_static_buffer_size - sizeof(response_packet_t);
        }

        request = (request_packet_t *) psa_adac_static_buffer_pointer;
        psa_adac_static_buffer_status = BUFFER_REQUEST;
    }

    return request;
}

int request_packet_release(request_packet_t *packet) {
    int ret = -1;
    if (psa_adac_static_buffer_status == BUFFER_REQUEST) {
        psa_adac_static_buffer_status = BUFFER_EMPTY;
        ret = 0;
    }

    return ret;
}

response_packet_t *response_packet_build(uint16_t status, uint8_t *data, size_t data_size) {
    response_packet_t *response = NULL;
    if ((psa_adac_static_buffer_status == BUFFER_EMPTY) &&
        (data_size <= (psa_adac_static_buffer_size - sizeof(response_packet_t)))) {
        response = (response_packet_t *) psa_adac_static_buffer_pointer;
        response->status = status;
        response->data_count = data_size / 4UL;
        (void) memcpy((void *) response->data, (void *) data, data_size);
        // TODO: Fill with 0s
        psa_adac_static_buffer_status = BUFFER_RESPONSE;
    }

    return response;
}

response_packet_t *response_packet_lock(size_t *max_data_size) {
    response_packet_t *response = NULL;
    if (psa_adac_static_buffer_status == BUFFER_EMPTY) {
        if (max_data_size != NULL) {
            *max_data_size = psa_adac_static_buffer_size - sizeof(response_packet_t);
        }
        response = (response_packet_t *) psa_adac_static_buffer_pointer;
        psa_adac_static_buffer_status = BUFFER_RESPONSE;
    }

    return response;
}

int response_packet_release(response_packet_t *packet) {
    int ret = -1;
    if (psa_adac_static_buffer_status == BUFFER_RESPONSE) {
        psa_adac_static_buffer_status = BUFFER_EMPTY;
        ret = 0;
    }

    return ret;
}
