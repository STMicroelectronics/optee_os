/*
 * Copyright (c) 2020-2023 Arm Limited. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include "psa_adac_debug.h"
#include "platform/msg_interface.h"
#include "platform/platform.h"

int psa_adac_detect_debug_request(void)
{
    /* Code me */
}

void psa_adac_acknowledge_debug_request(void)
{
    /* Code me */
}

int msg_interface_init(void *ctx, uint8_t buffer[], size_t buffer_size)
{
    /* Code me */
}

int msg_interface_free(void *ctx)
{
    /* Code me */
}

int request_packet_send(void)
{
    /* Code me */
}

request_packet_t *request_packet_receive(void *ctx)
{
    /* Code me */
}

int response_packet_send(response_packet_t *p)
{
    /* Code me */
}

response_packet_t *response_packet_receive(void)
{
    /* Code me */
}
