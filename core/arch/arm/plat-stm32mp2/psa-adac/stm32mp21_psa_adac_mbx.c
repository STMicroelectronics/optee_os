// SPDX-License-Identifier: BSD-3-Clause
/*
 * Copyright (c) 2020-2025 Arm Limited. All rights reserved.
 * Copyright (c) 2025 STMicroelectronics - All Rights Reserved
 */

#include <compiler.h>
#include "psa_adac_debug.h"
#include "platform/msg_interface.h"
#include "platform/platform.h"

int psa_adac_detect_debug_request(void)
{
	/* Code me */
	return PSA_ERROR_NOT_SUPPORTED;
}

void psa_adac_acknowledge_debug_request(void)
{
	/* Code me */
}

int msg_interface_init(uint8_t buffer[] __unused, size_t buffer_size __unused)
{
	/* Code me */
	return PSA_ERROR_NOT_SUPPORTED;
}

int msg_interface_free(void)
{
	/* Code me */
	return PSA_ERROR_NOT_SUPPORTED;
}

request_packet_t *request_packet_receive(void)
{
	/* Code me */
	return NULL;
}

int response_packet_send(response_packet_t *p __unused)
{
	/* Code me */
	return PSA_ERROR_NOT_SUPPORTED;
}
