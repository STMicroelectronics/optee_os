/* SPDX-License-Identifier: BSD-3-Clause */
/*
 * Copyright (c) 2022-2024, Arm Limited and Contributors. All rights reserved.
 * Copyright (C) 2025, STMicroelectronics - All Rights Reserved
 *
 */

#ifndef RSE_COMMS_H
#define RSE_COMMS_H

#include <stdint.h>

typedef int (*rse_send_callback_t)(const uint8_t *send_buffer, size_t size);
typedef int (*rse_recv_callback_t)(const uint8_t *receive_buffer, size_t *size);
typedef int (*rse_get_max_message_size_callback_t)(size_t *size);

int rse_register_cb(rse_send_callback_t send_cb, rse_recv_callback_t rcv_cb,
		    rse_get_max_message_size_callback_t size_cb);
void rse_set_client_id(uint16_t client_id);

#endif /* RSE_COMMS_H */
