// SPDX-License-Identifier: BSD-3-Clause
/*
 * Copyright (c) 2020-2025 Arm Limited. All rights reserved.
 * Copyright (c) 2025 STMicroelectronics - All Rights Reserved
 */

#include <assert.h>
#include <drivers/stm32_dbgmcu_mbx.h>
#include <psa_adac_debug.h>
#include <static_buffer_msg.h>
#include <tee_api_types.h>
#include "platform/msg_interface.h"
#include "platform/platform.h"

/* "STDA" is 0x53544441 'S' 'T' 'D' 'A' ASCII values */
#define STM32_DEBUG_REQUEST 0x53544441

static TEE_Result stm32_dbgmcu_mbx_receive(uint32_t *request_packet,
					   size_t max_length, size_t *length)
{
	TEE_Result res = TEE_ERROR_GENERIC;
	uint32_t max_words = max_length / sizeof(uint32_t);
	uint32_t data_count = max_words;
	uint32_t data = 0;
	uint32_t i = 0;

	for (i = 0; i < max_words && i < data_count + 2; i++) {
		res = stm32_dbgmcu_mbx_read_auth_host(&data, 1000);
		if (res) {
			EMSG("Failed to read from mailbox: %#" PRIx32, res);
			return res;
		}
		request_packet[i] = data;

		if (i == 0) { /* request_packet->command */
			DMSG("Receive request %u", data >> 16);
		} else if (i == 1) { /* request_packet->data_count in bytes */
			if (!data) {
				DMSG("Request without payload");
				goto recv_end;
			} else {
				data_count = data / sizeof(uint32_t);
				DMSG("Expecting %u words", data_count);
			}
		}
	}
recv_end:
	if (i == max_words) {
		EMSG("No more space");
		return TEE_ERROR_EXCESS_DATA;
	}

	*length = (i + 1) * sizeof(uint32_t);

	DMSG("%u words/%lu bytes received", (i + 1), *length);

	return TEE_SUCCESS;
}

static TEE_Result stm32_dbgmcu_mbx_send(uint32_t *response_packet,
					size_t length)
{
	TEE_Result res = TEE_ERROR_GENERIC;
	uint32_t data_count = length / sizeof(uint32_t);
	uint32_t data = 0;
	uint32_t i = 0;

	for (i = 0; i < data_count; i++) {
		data = response_packet[i];
		res = stm32_dbgmcu_mbx_write_auth_dev(data, 1000);
		if (res) {
			EMSG("Failed to write into mailbox: %#" PRIx32, res);
			return res;
		}
	}

	DMSG("%u words/%lu bytes sent", i, i * sizeof(uint32_t));

	return TEE_SUCCESS;
}

int psa_adac_detect_debug_request(void)
{
	TEE_Result res = TEE_ERROR_GENERIC;
	uint32_t value = 0;

	DMSG("Check debug request");

	res = stm32_dbgmcu_mbx_read_auth_host(&value, 1000);
	if (!res && value == STM32_DEBUG_REQUEST) {
		DMSG("Debug request received");
		return 1;
	}

	return 0;
}

void psa_adac_acknowledge_debug_request(void)
{
	TEE_Result res = TEE_ERROR_GENERIC;

	/* Needed to re-open up the access to the DBGMCU through AP0 */
	res = stm32_dbgmcu_mbx_write_auth_dev(STM32_DEBUG_REQUEST, 1000);
	if (!res)
		DMSG("Debug request acknowledged");
	else
		EMSG("Debug request NOT acknowledged");
}

int msg_interface_init(uint8_t buffer[], size_t buffer_size)
{
	return psa_adac_static_buffer_msg_init(buffer, buffer_size);
}

int msg_interface_free(void)
{
	return psa_adac_static_buffer_msg_release();
}

request_packet_t *request_packet_receive(void)
{
	TEE_Result res = TEE_ERROR_GENERIC;
	size_t max_length = 0;
	size_t length = 0;
	request_packet_t *p = request_packet_lock(&max_length);

	if (!p) {
		EMSG("Failed to lock request packet for reception");
		return NULL;
	}

	DMSG("Waiting for request");

	res = stm32_dbgmcu_mbx_receive((uint32_t *)p, max_length, &length);
	if (res) {
		EMSG("Receive failure: %#" PRIx32, res);
		request_packet_release(p);
		return NULL;
	}

	DMSG("Request received (%lu bytes)", length);

	return p;
}

int response_packet_send(response_packet_t *p)
{
	TEE_Result res = TEE_ERROR_GENERIC;
	size_t length = 0;

	if (!p)
		p = psa_adac_static_buffer_msg_get_response();

	/* p can't be NULL with static_buffer_msg API */
	assert(p);
	length = sizeof(p) + p->data_count * sizeof(uint32_t);

	DMSG("Sending response");

	res = stm32_dbgmcu_mbx_send((uint32_t *)p, length);
	if (res)
		EMSG("Send failure: %#" PRIx32, res);

	return res;
}
