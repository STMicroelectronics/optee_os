/*
 * Copyright (c) 2020 Arm Limited. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef PSA_ADAC_STATIC_BUFFER_MSG_H
#define PSA_ADAC_STATIC_BUFFER_MSG_H

#include <psa_adac.h>

#include <stddef.h>
#include <stdint.h>

int psa_adac_static_buffer_msg_init(uint8_t *buffer, size_t size);
int psa_adac_static_buffer_msg_release(void);

request_packet_t *psa_adac_static_buffer_msg_get_request(void);
response_packet_t *psa_adac_static_buffer_msg_get_response(void);

#endif //PSA_ADAC_STATIC_BUFFER_MSG_H
