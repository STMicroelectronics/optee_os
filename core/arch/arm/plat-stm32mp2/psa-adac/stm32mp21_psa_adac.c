// SPDX-License-Identifier: BSD-3-Clause
/*
 * Copyright (c) 2020-2025 Arm Limited. All rights reserved.
 * Copyright (c) 2025 STMicroelectronics - All Rights Reserved
 */

#include <drivers/stm32_bsec.h>
#include <psa_adac_config.h>
#include <psa_adac_debug.h>
#include <psa_adac_sda.h>
#include <stm32_util.h>
#include <stdlib_ext.h>
#include <string.h>
#include <tee_api_types.h>
#include <utee_defines.h>
#include "platform/platform.h"
#include "platform/msg_interface.h"

/* TODO Discovery template */
uint8_t discovery_template[] = {
	// @+00 (12 bytes) psa_auth_version: 1.0
	0x00, 0x00, 0x01, 0x00, /* _reserved + type_id = 0x0001 */
	0x02, 0x00, 0x00, 0x00, /* length = 0x00000002 */
	0x01, 0x00, 0x00, 0x00, /* value = 0x0001 + padding */
	// @+12 (12 bytes) vendor_id: {0x00, 0x20} => STMicroelectronics
	0x00, 0x00, 0x02, 0x00, /* _reserved + type_id = 0x0002 */
	0x02, 0x00, 0x00, 0x00, /* length = 0x00000002 */
	0x00, 0x20, 0x00, 0x00, /* value = 0x0020 + padding */
	// @+24 (12 bytes) soc_class: [0x03, 0x05, 0x00, 0x00] => STM32MP21xx
	0x00, 0x00, 0x03, 0x00, /* _reserved + type_id = 0x0003 */
	0x04, 0x00, 0x00, 0x00, /* length = 0x00000004 */
	0x03, 0x05, 0x00, 0x00, /* value = 0x00000503 + padding */
	// @+36 (24 bytes) soc_id: [0x00] * 16
	0x00, 0x00, 0x04, 0x00, /* _reserved + type_id = 0x0004 */
	0x10, 0x00, 0x00, 0x00, /* length = 0x00000010 */
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, /* va... */
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, /* ...lue */
	// @+60 (12 bytes) psa_lifecycle: PSA_LIFECYCLE_SECURED 0x3000
	0x00, 0x00, 0x08, 0x00, /* _reserved + type_id = 0x0008 */
	0x02, 0x00, 0x00, 0x00, /* length = 0x00000002 */
	0x00, 0x30, 0x00, 0x00, /* value = 0x3000 */
	// @+72 (12 bytes) token_formats: [{0x00, 0x02} (token_psa_debug)]
	0x00, 0x00, 0x00, 0x01, /* _reserved + type_id = 0x0010 */
	0x02, 0x00, 0x00, 0x00, /* length = 0x00000002 */
	0x00, 0x02, 0x00, 0x00, /* value = 0x0200 + padding */
	// @+84 (12 bytes) cert_formats: [{0x01, 0x02} (cert_psa_debug)]
	0x00, 0x00, 0x01, 0x01, /* _reserved + type_id = 0x0011 */
	0x02, 0x00, 0x00, 0x00, /* length = 0x00000002 */
	0x01, 0x02, 0x00, 0x00, /* value = 0x0201 + padding */
	// @+96 (12 bytes) cryptosystems: [ ECDSA_P256_SHA256 ]
	0x00, 0x00, 0x02, 0x01, /* _reserved + type_id = 0x0012 */
	0x01, 0x00, 0x00, 0x00, /* length = 0x00000001 */
	0x01, 0x00, 0x00, 0x00, /* value = 0x01 + padding */
};

size_t discovery_template_len = sizeof(discovery_template) -
				(sizeof(discovery_template) % 4);
/* -- End Discovery template -- */

/* Needed for initialization of authentication context */
static uint8_t buffer[512];
static uint8_t messages[512];
/* -- End Authentication context -- */

void psa_adac_platform_init(void)
{
}

size_t psa_adac_platform_discovery(uint8_t *reply, size_t reply_size)
{
	TEE_Result res = TEE_ERROR_GENERIC;
	uint32_t soc_class_ofst = 24 + 2 * sizeof(uint32_t);
	uint32_t soc_id_ofst = 36 + 2 * sizeof(uint32_t);
	uint32_t soc_class = 0;
	uint8_t *soc_id = NULL;

	if (reply_size < discovery_template_len)
		return 0;

	memcpy(reply, discovery_template, discovery_template_len);

	soc_class = stm32mp_syscfg_get_chip_dev_id();
	if (soc_class)
		memcpy(reply + soc_class_ofst, &soc_class, sizeof(uint32_t));

	/* uid is 96 bits, aka 12 bytes (soc_id is 16 bytes) */
	res = stm32_bsec_read_otp_range_by_name("uid_otp", 12, &soc_id);
	if (res)
		EMSG("Can't get UID to update discovery template");
	if (soc_id) {
		memcpy(reply + soc_id_ofst, soc_id, 12);
		free_wipe(soc_id);
	}

	return discovery_template_len;
}

adac_status_t psa_adac_change_life_cycle_state(uint8_t *input __unused,
					       size_t input_size __unused)
{
	return ADAC_UNSUPPORTED;
}

void psa_adac_platform_lock(void)
{
	/*
	 * Not all targets have the capability to lock debug access after it is
	 * unlocked without going through a Reset cycle (usually a Cold reset)
	 */
}

int psa_adac_platform_check_token(uint8_t *token __unused,
				  size_t token_size __unused)
{
	/* TODO: platform can reject the token */
	return PSA_SUCCESS;
}

int psa_adac_platform_check_certificate(uint8_t *crt __unused,
					size_t crt_size __unused)
{
	/* TODO: platform can reject the certificate */
	return PSA_SUCCESS;
}

int psa_adac_apply_permissions(uint8_t permissions_mask[16])
{
	TEE_Result res = TEE_ERROR_GENERIC;
	uint32_t perm_mask = 0;

	/* Permission mask on this platform is defined on 32 bits */
	memcpy(&perm_mask, permissions_mask, sizeof(uint32_t));

	DMSG("perm_mask=%#" PRIx32, perm_mask);

	res = stm32_bsec_write_debug_conf(perm_mask);

	return (!res) ? PSA_SUCCESS : PSA_ERROR_HARDWARE_FAILURE;
}

void platform_init(void)
{
}

int psa_adac_start_secure_debug(void)
{
	uint8_t rotpk_type[] = { ECDSA_P256_SHA256, };
	size_t rotpkh_len[] =  { TEE_SHA256_HASH_SIZE, };
	uint8_t *rotpkh = NULL;
	authentication_context_t auth_ctx = { };
	TEE_Result res = TEE_ERROR_GENERIC;

	if (!psa_adac_detect_debug_request()) {
		IMSG("No secure debug connection");
		return TEE_ERROR_CANCEL;
	}

	IMSG("Secure debug connection established");

	/* This may fail depending on buffer status */
	if (msg_interface_init(messages, sizeof(messages)))
		return TEE_ERROR_BAD_STATE;

	/* This may fail depending on crypto init */
	if (psa_adac_init()) {
		msg_interface_free();
		EMSG("Failed to initialize ADAC library, abort secure debug");
		return TEE_ERROR_NOT_SUPPORTED;
	}

	stm32_bsec_mp21_ap0_unlock();

	psa_adac_acknowledge_debug_request();

	res = stm32_bsec_read_otp_range_by_name("oem_adac_rotpkh",
						rotpkh_len[0], &rotpkh);
	if (res) {
		EMSG("Can't get ADAC ROTPK: %#" PRIx32, res);
		goto out_free;
	}

	authentication_context_init(&auth_ctx, buffer, sizeof(buffer),
				    PSA_ALG_SHA_256, &rotpkh, rotpkh_len,
				    rotpk_type, 1);

	IMSG("Starting secure debug authentication");

	/*
	 * This returns 1 when authentication is done (successfully or not)
	 * so need to check context state instead.
	 */
	authentication_handle(&auth_ctx);
	res = auth_ctx.state == AUTH_SUCCESS ? TEE_SUCCESS :
					       TEE_ERROR_BAD_STATE;

	if (!res)
		IMSG("Secure debug authentication success");
	else
		EMSG("Secure debug authentication failure");

	free_wipe(rotpkh);
	rotpkh = NULL;
out_free:
	msg_interface_free();

	return res;
}
