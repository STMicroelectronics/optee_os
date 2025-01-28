/* SPDX-License-Identifier: BSD-3-Clause */
/*
 * Copyright (c) 2022-2025 Arm Limited. All rights reserved.
 * Copyright (c) 2025 STMicroelectronics - All Rights Reserved
 */

#ifndef __MSG_INTERFACE_H__
#define __MSG_INTERFACE_H__

#include <psa_adac.h>
#include "platform/platform.h"
#include <stddef.h>

int msg_interface_init(uint8_t buffer[], size_t size);
int msg_interface_free(void);

request_packet_t *request_packet_lock(size_t *max_data_size);
response_packet_t *response_packet_lock(size_t *max_data_size);
int response_packet_release(response_packet_t *packet);
int request_packet_release(request_packet_t *packet);

request_packet_t *request_packet_receive(void);
response_packet_t *response_packet_build(uint16_t status, uint8_t *data,
					 size_t data_size);
int response_packet_send(response_packet_t *packet);

#endif /* __MSG_INTERFACE_H__ */
