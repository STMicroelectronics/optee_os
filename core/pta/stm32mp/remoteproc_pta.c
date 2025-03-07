// SPDX-License-Identifier: BSD-2-Clause
/*
 * Copyright (C) 2023, STMicroelectronics - All Rights Reserved
 */

#include <crypto/crypto.h>
#include <drivers/clk.h>
#include <drivers/rstctrl.h>
#include <drivers/stm32_bsec.h>
#include <drivers/stm32_remoteproc.h>
#include <drivers/stm32mp_dt_bindings.h>
#include <drivers/stm32mp1_rcc.h>
#include <initcall.h>
#include <kernel/pseudo_ta.h>
#include <kernel/user_ta.h>
#include <remoteproc_pta.h>
#include <stdlib_ext.h>
#include <string.h>
#include <string_ext.h>

#include "rproc_pub_key.h"

#define PTA_NAME	"remoteproc.pta"

/*
 * UUID of the remoteproc Trusted application authorized to communicate with
 * the remoteproc pseudo TA. The UID should match the one defined in the
 * ta_remoteproc.h header file.
 */
#define TA_REMOTEPROC_UUID \
	{ 0x80a4c275, 0x0a47, 0x4905, \
		{ 0x82, 0x85, 0x14, 0x86, 0xa9, 0x77, 0x1a, 0x08} }

#define PTA_ECC_DER_SIZE 91
#define PTA_ECC_X_SIZE   32
#define PTA_ECC_Y_SIZE   32

#define PTA_RSA_DER_SIZE 294
#define PTA_RSA_MOD_SIZE 256
#define PTA_RSA_E_SIZE   3

/*
 * AES test key for decryption.
 */
static uint8_t aes_test_key[TEE_AES_MAX_KEY_SIZE] = {
	0x2A, 0x2B, 0x01, 0xB1, 0x75, 0xFF, 0xE8, 0x26,
	0x6D, 0x9E, 0x8D, 0x31, 0xC5, 0x7E, 0x93, 0xB5,
	0x1F, 0xE0, 0x55, 0x93, 0x49, 0xA6, 0x9A, 0x4C,
	0x4F, 0xF6, 0x36, 0x23, 0x33, 0x3E, 0xFB, 0x3D
};

static const uint8_t der_ecc_pkey_header[] = {
	0x30, 0x59, 0x30, 0x13, 0x06, 0x07, 0x2a, 0x86,
	0x48, 0xce, 0x3d, 0x02, 0x01, 0x06, 0x08, 0x2a,
	0x86, 0x48, 0xce, 0x3d, 0x03, 0x01, 0x07, 0x03,
	0x42, 0x00, 0x04
};

static const uint8_t der_rsa256_pkey_header[] = {
	0x30, 0x82, 0x01, 0x22, 0x30, 0x0d, 0x06, 0x09,
	0x2a, 0x86, 0x48, 0x86, 0xf7, 0x0d, 0x01, 0x01,
	0x01, 0x05, 0x00, 0x03, 0x82, 0x01, 0x0f, 0x00,
	0x30, 0x82, 0x01, 0x0a, 0x02, 0x82, 0x01, 0x01
};

static const uint8_t der_rsa256_pkey_mod_header[] = { 0x02, 0x03 };

/*
 * Firmware states
 * REMOTEPROC_OFF: firmware is off
 * REMOTEPROC_ON: firmware is running
 */
enum rproc_load_state {
	REMOTEPROC_OFF = 0,
	REMOTEPROC_ON,
};

/* Currently supporting a single remote processor instance */
static enum rproc_load_state rproc_ta_state = REMOTEPROC_OFF;

static bool is_key_zero(const uint8_t *buf, size_t size)
{
	size_t i = 0;
	uint8_t result = 0;

	for (i = 0; i < size; i++)
		result |= buf[i];

	return (result == 0);
}

static TEE_Result rproc_pta_capabilities(uint32_t pt,
					 TEE_Param params[TEE_NUM_PARAMS])
{
	const uint32_t exp_pt = TEE_PARAM_TYPES(TEE_PARAM_TYPE_VALUE_INPUT,
						TEE_PARAM_TYPE_VALUE_OUTPUT,
						TEE_PARAM_TYPE_VALUE_OUTPUT,
						TEE_PARAM_TYPE_NONE);

	if (pt != exp_pt)
		return TEE_ERROR_BAD_PARAMETERS;

	if (!stm32_rproc_get(params[0].value.a))
		return TEE_ERROR_NOT_SUPPORTED;

	/* Support ELF format and encrypted ELF format*/
	params[1].value.a = PTA_RPROC_HWCAP_FMT_ELF |
			    PTA_RPROC_HWCAP_FMT_ENC_ELF;

	/*
	 * Due to stm32mp1 pager, secure memory is too expensive. Support hash
	 * protected image only, so that firmware image can be loaded from
	 * non-secure memory.
	 */
	params[2].value.a = PTA_RPROC_HWCAP_PROT_HASH_TABLE;

	return TEE_SUCCESS;
}

static TEE_Result rproc_alloc_and_get_otp(const char *name, uint8_t **value,
					  size_t value_len)
{
	TEE_Result res = TEE_SUCCESS;
	uint8_t otp_bit_offset = 0;
	uint32_t *value_buf = NULL;
	size_t otp_bit_size = 0;
	uint32_t otp_start = 0;
	size_t otp_length = 0;
	uint32_t otp_id = 0;

	res = stm32_bsec_find_otp_in_nvmem_layout(name, &otp_start,
						  &otp_bit_offset,
						  &otp_bit_size);
	if (res) {
		EMSG("Can't find %s", name);
		return res;
	}

	if (otp_bit_offset || otp_bit_size != value_len * CHAR_BIT) {
		EMSG("Bad OTP alignment");
		return TEE_ERROR_GENERIC;
	}

	otp_length = value_len / sizeof(uint32_t);
	value_buf = calloc(otp_length, sizeof(uint32_t));
	if (!value_buf)
		return TEE_ERROR_OUT_OF_MEMORY;

	*value = (uint8_t *)value_buf;

	for (otp_id = otp_start; otp_id < otp_start + otp_length;
	     otp_id++, value_buf++) {
		/* Read value in OTP */
		res = stm32_bsec_read_otp(value_buf, otp_id);
		if (res)
			goto clean_value;
	}

	return TEE_SUCCESS;

clean_value:
	free_wipe(*value);

	return res;
}

static TEE_Result rproc_pta_decrypt_aes(uint32_t enc_algo, uint8_t *iv,
					uint8_t *buff, size_t len)
{
	size_t key_len = TEE_AES_MAX_KEY_SIZE;
	TEE_Result res = TEE_ERROR_GENERIC;
	uint8_t *key = NULL;
	void *ctx = NULL;

	if (IS_ENABLED(CFG_REMOTEPROC_ENC_TESTKEY)) {
		key = aes_test_key;
	} else {
		res = rproc_alloc_and_get_otp("oem_rproc_enc_key", &key,
					      key_len);
		if (res)
			return res;
	}

	if (is_key_zero(key, key_len)) {
		EMSG("null encrypted key not supported");
		res = TEE_ERROR_SECURITY;
		goto clean_key;
	}

	res = crypto_cipher_alloc_ctx(&ctx, enc_algo);
	if (res)
		goto clean_key;

	res = crypto_cipher_init(ctx, TEE_MODE_DECRYPT, key, key_len,
				 NULL, 0, iv, TEE_AES_BLOCK_SIZE);
	if (res)
		goto clean_ctx;

	/* In-place decryption in the destination memory*/
	res = crypto_cipher_update(ctx, TEE_MODE_DECRYPT, true, buff,
				   len, buff);
	if (res)
		goto clean_ctx;

	crypto_cipher_final(ctx);

clean_ctx:
	crypto_cipher_free_ctx(ctx);
clean_key:
	if (!IS_ENABLED(CFG_REMOTEPROC_ENC_TESTKEY))
		free_wipe(key);

	return res;
}

static TEE_Result rproc_pta_load_segment(uint32_t pt,
					 TEE_Param params[TEE_NUM_PARAMS])
{
	const uint32_t exp_pt = TEE_PARAM_TYPES(TEE_PARAM_TYPE_VALUE_INPUT,
						TEE_PARAM_TYPE_MEMREF_INPUT,
						TEE_PARAM_TYPE_VALUE_INPUT,
						TEE_PARAM_TYPE_MEMREF_INPUT);
	struct rproc_pta_seg_info *seg_info = params[3].memref.buffer;
	TEE_Result res = TEE_ERROR_GENERIC;
	paddr_t pa = 0;
	void *dst = NULL;
	uint8_t *src = params[1].memref.buffer;
	size_t size = params[1].memref.size;
	paddr_t da = (paddr_t)reg_pair_to_64(params[2].value.b,
					     params[2].value.a);

	if (pt != exp_pt)
		return TEE_ERROR_BAD_PARAMETERS;

	if (!seg_info || params[3].memref.size != sizeof(*seg_info))
		return TEE_ERROR_BAD_PARAMETERS;

	if (seg_info->hash_algo != TEE_ALG_SHA256)
		return TEE_ERROR_NOT_SUPPORTED;

	if (rproc_ta_state != REMOTEPROC_OFF)
		return TEE_ERROR_BAD_STATE;

	/* Get the physical address in local context mapping */
	res = stm32_rproc_da_to_pa(params[0].value.a, da, size, &pa);
	if (res)
		return res;

	if (stm32_rproc_map(params[0].value.a, pa, size, &dst)) {
		EMSG("Can't map region %#"PRIxPA" size %zu", pa, size);
		return TEE_ERROR_GENERIC;
	}

	/* Copy the segment to the remote processor memory */
	memcpy(dst, src, size);

	/* Verify that loaded segment is valid */
	res = hash_sha256_check(seg_info->hash, dst, size);
	if (res)
		goto clean_mem;

	if (res || !seg_info->enc_algo)
		goto unmap;

	/* Decrypt the segment copied in destination memory */
	res = rproc_pta_decrypt_aes(seg_info->enc_algo, seg_info->iv,
				    dst, size);
	if (res == TEE_SUCCESS)
		goto unmap;

clean_mem:
	memzero_explicit(dst, size);
unmap:
	stm32_rproc_unmap(params[0].value.a, dst, size);

	return res;
}

static TEE_Result rproc_pta_set_memory(uint32_t pt,
				       TEE_Param params[TEE_NUM_PARAMS])
{
	const uint32_t exp_pt = TEE_PARAM_TYPES(TEE_PARAM_TYPE_VALUE_INPUT,
						TEE_PARAM_TYPE_VALUE_INPUT,
						TEE_PARAM_TYPE_VALUE_INPUT,
						TEE_PARAM_TYPE_VALUE_INPUT);
	TEE_Result res = TEE_ERROR_GENERIC;
	paddr_t pa = 0;
	void *dst = NULL;
	paddr_t da = params[1].value.a;
	size_t size = params[2].value.a;
	uint8_t value = params[3].value.a && 0xFF;

	if (pt != exp_pt)
		return TEE_ERROR_BAD_PARAMETERS;

	if (rproc_ta_state != REMOTEPROC_OFF)
		return TEE_ERROR_BAD_STATE;

	/* Get the physical address in CPU mapping */
	res = stm32_rproc_da_to_pa(params[0].value.a, da, size, &pa);
	if (res)
		return res;

	res = stm32_rproc_map(params[0].value.a, pa, size, &dst);
	if (res) {
		EMSG("Can't map region %#"PRIxPA" size %zu", pa, size);
		return TEE_ERROR_GENERIC;
	}

	memset(dst, value, size);

	return stm32_rproc_unmap(params[0].value.a, dst, size);
}

static TEE_Result rproc_pta_da_to_pa(uint32_t pt,
				     TEE_Param params[TEE_NUM_PARAMS])
{
	const uint32_t exp_pt = TEE_PARAM_TYPES(TEE_PARAM_TYPE_VALUE_INPUT,
						TEE_PARAM_TYPE_VALUE_INPUT,
						TEE_PARAM_TYPE_VALUE_INPUT,
						TEE_PARAM_TYPE_VALUE_OUTPUT);
	TEE_Result res = TEE_ERROR_GENERIC;
	paddr_t da = params[1].value.a;
	size_t size = params[2].value.a;
	paddr_t pa = 0;

	DMSG("Conversion for address %#"PRIxPA" size %zu", da, size);

	if (pt != exp_pt)
		return TEE_ERROR_BAD_PARAMETERS;

	/* Target address is expected 32bit, ensure 32bit MSB are zero */
	if (params[1].value.b || params[2].value.b)
		return TEE_ERROR_BAD_PARAMETERS;

	res = stm32_rproc_da_to_pa(params[0].value.a, da, size, &pa);
	if (res)
		return res;

	reg_pair_from_64((uint64_t)pa, &params[3].value.b, &params[3].value.a);

	return TEE_SUCCESS;
}

static TEE_Result rproc_pta_start(uint32_t pt,
				  TEE_Param params[TEE_NUM_PARAMS])
{
	TEE_Result res = TEE_ERROR_GENERIC;
	const uint32_t exp_pt = TEE_PARAM_TYPES(TEE_PARAM_TYPE_VALUE_INPUT,
						TEE_PARAM_TYPE_NONE,
						TEE_PARAM_TYPE_NONE,
						TEE_PARAM_TYPE_NONE);

	if (pt != exp_pt)
		return TEE_ERROR_BAD_PARAMETERS;

	if (rproc_ta_state != REMOTEPROC_OFF)
		return TEE_ERROR_BAD_STATE;

	res = stm32_rproc_start(params[0].value.a);
	if (res)
		return res;

	rproc_ta_state = REMOTEPROC_ON;

	return TEE_SUCCESS;
}

static TEE_Result rproc_pta_stop(uint32_t pt,
				 TEE_Param params[TEE_NUM_PARAMS])
{
	const uint32_t exp_pt = TEE_PARAM_TYPES(TEE_PARAM_TYPE_VALUE_INPUT,
						TEE_PARAM_TYPE_NONE,
						TEE_PARAM_TYPE_NONE,
						TEE_PARAM_TYPE_NONE);
	TEE_Result res = TEE_ERROR_GENERIC;

	if (pt != exp_pt)
		return TEE_ERROR_BAD_PARAMETERS;

	if (rproc_ta_state != REMOTEPROC_ON)
		return TEE_ERROR_BAD_STATE;

	res = stm32_rproc_stop(params[0].value.a);
	if (res)
		return res;

	rproc_ta_state = REMOTEPROC_OFF;

	return TEE_SUCCESS;
}

static TEE_Result rproc_compute_hash(uint8_t *digest, const uint8_t *hash1,
				     size_t hash1_size, const uint8_t *hash2,
				     size_t hash2_size)
{
	TEE_Result res = TEE_SUCCESS;
	void *ctx = NULL;

	res = crypto_hash_alloc_ctx(&ctx, TEE_ALG_SHA256);
	if (res)
		return res;
	res = crypto_hash_init(ctx);
	if (res)
		goto out;
	res = crypto_hash_update(ctx, hash1, hash1_size);
	if (res)
		goto out;
	res = crypto_hash_update(ctx, hash2, hash2_size);
	if (res)
		goto out;
	res = crypto_hash_final(ctx, digest, TEE_SHA256_HASH_SIZE);

out:
	crypto_hash_free_ctx(ctx);

	return res;
}

static TEE_Result parse_der_ecc_public_key(uint8_t *der, size_t der_len,
					   struct ecc_public_key *key,
					   uint8_t *key_hash)
{
	size_t h_len = sizeof(der_ecc_pkey_header);
	TEE_Result res = TEE_ERROR_GENERIC;

	/*
	 * We does not use the mbedtls lib that contains helper to parse the DER
	 * key and extract the ECC key. A static approach is used supporting
	 * only DER format for the ECC public key with the ECDSA P256 algorithm.
	 * The expected DER format is starting with following header:
	 * { 0x30, 0x59, 0x30, 0x13, 0x06, 0x07, 0x2a, 0x86,
	 *   0x48, 0xce, 0x3d, 0x02, 0x01, 0x06, 0x08, 0x2a,
	 *   0x86, 0x48, 0xce, 0x3d, 0x03, 0x01, 0x07, 0x03,
	 *   0x42, 0x00, 0x04 }
	 *
	 * 30 59 # Sequence length 0x59 - 91 bytes long
	 *    30 13 # Sequence length 0x13 - 21 bytes long
	 *       06 07 2a8648ce3d0201  # OID - 7 bytes long - ECC
	 *       06 08 2a8648ce3d030107 # OID - 8 bytes long -
	 *				ECDSA P256 curve
	 *    03 42 # Bit stream - 0x42 (66 bytes long)
	 *       0004 # Identifies public key
	 * The last 64 bytes contain the X (32 bytes) and Y (32 bytes) key
	 * coordinates.
	 */
	if (der_len != PTA_ECC_DER_SIZE) {
		EMSG("Invalid public key size");
		return TEE_ERROR_BAD_FORMAT;
	}

	if (consttime_memcmp(der, der_ecc_pkey_header, h_len != 0)) {
		EMSG("Invalid public key format");
		return TEE_ERROR_BAD_FORMAT;
	}

	key->curve = TEE_ECC_CURVE_NIST_P256;

	/* Extract x and y coordinates */
	res = crypto_bignum_bin2bn(der + h_len, PTA_ECC_X_SIZE, key->x);
	if (res)
		return res;

	res = crypto_bignum_bin2bn(der + h_len + PTA_ECC_X_SIZE, PTA_ECC_Y_SIZE,
				   key->y);
	if (res)
		return res;

	return rproc_compute_hash(key_hash, der + h_len, PTA_ECC_X_SIZE,
				  der + h_len + PTA_ECC_X_SIZE, PTA_ECC_Y_SIZE);
}

static TEE_Result
rproc_pta_verify_ecc_signature(TEE_Param *hash, TEE_Param *sig,
			       struct rproc_pta_key_info *keyinfo)
{
	uint8_t publickey_hash[TEE_SHA256_HASH_SIZE] = { };
	size_t hash_size = hash->memref.size;
	size_t sig_size = sig->memref.size;
	size_t hash_len = TEE_SHA256_HASH_SIZE;
	TEE_Result res = TEE_ERROR_GENERIC;
	struct ecc_public_key key = { };
	uint8_t *key_hash = NULL;

	if (!IS_ENABLED(CFG_REMOTEPROC_PUB_KEY_VERIFY))
		return TEE_ERROR_NOT_SUPPORTED;

	/* Only support public key provided with the image in keyinfo */
	res = crypto_acipher_alloc_ecc_public_key(&key,
						  TEE_TYPE_ECDSA_PUBLIC_KEY,
						  sig_size);
	if (res)
		return res;

	res = parse_der_ecc_public_key(keyinfo->info, keyinfo->info_size, &key,
				       publickey_hash);
	if (res) {
		EMSG("Invalid public key");
		goto out;
	}

	res = rproc_alloc_and_get_otp("oem_rproc_pkh", &key_hash, hash_len);
	if (res)
		goto out;

	if (consttime_memcmp(key_hash, publickey_hash,
			     TEE_SHA256_HASH_SIZE) != 0) {
		EMSG("Invalid public key hash");
		res = TEE_ERROR_SECURITY;
		goto out;
	}

	res = crypto_acipher_ecc_verify(keyinfo->algo, &key,
					hash->memref.buffer, hash_size,
					sig->memref.buffer, sig_size);

out:
	free(key_hash);
	crypto_acipher_free_ecc_public_key(&key);

	return res;
}

static TEE_Result parse_der_rsa_public_key(uint8_t *der, size_t der_len,
					   struct rsa_public_key *key,
					   uint8_t *key_hash)
{
	size_t h_len = sizeof(der_rsa256_pkey_header);
	uint8_t *mod = der + h_len + 1;
	uint8_t *exp = mod + PTA_RSA_MOD_SIZE + 2;
	TEE_Result res = TEE_ERROR_GENERIC;

	/*
	 * We do not use the mbedtls lib that contains helper to parse the DER
	 * key and extract the RSA key. A static approach is used supporting
	 * only DER format for the RSA public key with the ECDSA P256 algorithm.
	 * The expected DER format is starting with following header:
	 * { 0x30, 0x82, 0x01, 0x22, 0x30, 0x0d, 0x06, 0x09,
	 *   0x2a, 0x86, 0x48, 0x86, 0xf7, 0x0d, 0x01, 0x01,
	 *   0x01, 0x05, 0x00, 0x03, 0x82, 0x01, 0x0f, 0x00,
	 *   0x30, 0x82, 0x01, 0x0a, 0x02, 0x82, 0x01, 0x01,
	 *   0x00, <modulus binary data>,
	 *   0x02, 0x03, <exponent binary data> }
	 *
	 * 30 82 01 22 # Sequence length 0x82 - 290 bytes long
	 *    30 0d # Sequence length 0x0d - 13 bytes long
	 *       06 09 2a864886f70d010101 # OID - 9 bytes long - rsa (PKCS #1)
	 *       05 00 # Null Object
	 *    03 82 01 0f 00 # Bit stream - 271 bytes long
	 *       30 82 01 0a: # Sequence length 0x10a - 266 bytes long
	 *          02 82 01 01: #int modulus - 257 bytes long (0x00 + modulus)
	 *          02 03 : #int exponent - 3 bytes long
	 */

	if (der_len != PTA_RSA_DER_SIZE) {
		EMSG("Invalid public key size");
		return TEE_ERROR_BAD_FORMAT;
	}

	if (consttime_memcmp(der, der_rsa256_pkey_header, h_len) != 0 ||
	    consttime_memcmp(exp - 2, der_rsa256_pkey_mod_header,
			     sizeof(der_rsa256_pkey_mod_header)) != 0) {
		EMSG("Invalid public key format");
		return TEE_ERROR_BAD_FORMAT;
	}

	/* Extract the modulus and exponent */
	res = crypto_bignum_bin2bn(mod, PTA_RSA_MOD_SIZE, key->n);
	if (res)
		return res;

	res = crypto_bignum_bin2bn(exp, PTA_RSA_E_SIZE, key->e);
	if (res)
		return res;

	/* Compute the hash of the 256-bytes modulus + 3-bytes exponent */
	return rproc_compute_hash(key_hash, mod, PTA_RSA_MOD_SIZE,
				  exp, PTA_RSA_E_SIZE);
}

static TEE_Result
rproc_pta_verify_rsa_signature(TEE_Param *hash, TEE_Param *sig,
			       struct rproc_pta_key_info *keyinfo)
{
	uint32_t e = TEE_U32_TO_BIG_ENDIAN(rproc_pub_key_exponent);
	uint8_t publickey_hash[TEE_SHA256_HASH_SIZE] = { };
	size_t hash_size = hash->memref.size;
	size_t sig_size = sig->memref.size;
	size_t hash_len = TEE_SHA256_HASH_SIZE;
	TEE_Result res = TEE_ERROR_GENERIC;
	struct rsa_public_key key = { };
	uint32_t algo = keyinfo->algo;
	uint8_t *key_hash = NULL;

	res = crypto_acipher_alloc_rsa_public_key(&key, sig_size);
	if (res)
		return res;

	if (IS_ENABLED(CFG_REMOTEPROC_PUB_KEY_VERIFY)) {
		/* Use the public key provided with the image in keyinfo */
		res = parse_der_rsa_public_key(keyinfo->info,
					       keyinfo->info_size,
					       &key, publickey_hash);
		if (res)
			goto out;

		res = rproc_alloc_and_get_otp("oem_rproc_pkh", &key_hash,
					      hash_len);
		if (res)
			goto clean_key_hash;

		if (consttime_memcmp(key_hash, publickey_hash,
				     TEE_SHA256_HASH_SIZE) != 0) {
			EMSG("Invalid public key");
			res = TEE_ERROR_SECURITY;
			goto clean_key_hash;
		}
	} else {
		/* Use the public key embedded in OP-TEE */
		res = crypto_bignum_bin2bn((uint8_t *)&e, sizeof(e), key.e);
		if (res)
			goto out;

		res = crypto_bignum_bin2bn(rproc_pub_key_modulus,
					   rproc_pub_key_modulus_size, key.n);
		if (res)
			goto out;
	}

	res = crypto_acipher_rsassa_verify(algo, &key, hash_size,
					   hash->memref.buffer, hash_size,
					   sig->memref.buffer, sig_size);

clean_key_hash:
	if (key_hash)
		free_wipe(key_hash);
out:
	crypto_acipher_free_rsa_public_key(&key);

	return res;
}

static TEE_Result rproc_pta_verify_digest(uint32_t pt,
					  TEE_Param params[TEE_NUM_PARAMS])
{
	struct rproc_pta_key_info *keyinfo = NULL;
	const uint32_t exp_pt = TEE_PARAM_TYPES(TEE_PARAM_TYPE_VALUE_INPUT,
						TEE_PARAM_TYPE_MEMREF_INPUT,
						TEE_PARAM_TYPE_MEMREF_INPUT,
						TEE_PARAM_TYPE_MEMREF_INPUT);

	if (pt != exp_pt)
		return TEE_ERROR_BAD_PARAMETERS;

	if (!stm32_rproc_get(params[0].value.a))
		return TEE_ERROR_NOT_SUPPORTED;

	if (rproc_ta_state != REMOTEPROC_OFF)
		return TEE_ERROR_BAD_STATE;

	keyinfo = params[1].memref.buffer;

	if (!keyinfo ||
	    rproc_pta_keyinfo_size(keyinfo) != params[1].memref.size)
		return TEE_ERROR_BAD_PARAMETERS;

	switch (keyinfo->algo) {
	case TEE_ALG_RSASSA_PKCS1_V1_5_SHA256:
		return rproc_pta_verify_rsa_signature(&params[2], &params[3],
						      keyinfo);
	case TEE_ALG_ECDSA_SHA256:
		return rproc_pta_verify_ecc_signature(&params[2], &params[3],
						      keyinfo);
	default:
		EMSG("Unsupported algo %#x",  keyinfo->algo);
		return TEE_ERROR_NOT_SUPPORTED;
	}
}

static TEE_Result rproc_pta_tlv_param(uint32_t pt,
				      TEE_Param params[TEE_NUM_PARAMS])
{
	const uint32_t exp_pt = TEE_PARAM_TYPES(TEE_PARAM_TYPE_VALUE_INPUT,
						TEE_PARAM_TYPE_VALUE_INPUT,
						TEE_PARAM_TYPE_MEMREF_INPUT,
						TEE_PARAM_TYPE_NONE);
	uint32_t type_id = 0;
	uint32_t *paddr = NULL;
	uint32_t *sec = NULL;

	if (pt != exp_pt)
		return TEE_ERROR_BAD_PARAMETERS;

	if (rproc_ta_state != REMOTEPROC_OFF)
		return TEE_ERROR_BAD_STATE;

	type_id = params[1].value.a;

	switch (type_id) {
	case PTA_REMOTEPROC_TLV_BOOTADDR:
		if (params[2].memref.size != PTA_REMOTEPROC_TLV_BOOTADDR_LGTH)
			return TEE_ERROR_CORRUPT_OBJECT;

		paddr = params[2].memref.buffer;

		return stm32_rproc_set_boot_address(params[0].value.a, *paddr);

	case PTA_REMOTEPROC_TLV_BOOT_SEC:
		if (params[2].memref.size != PTA_REMOTEPROC_TLV_BOOT_SEC_LGTH)
			return TEE_ERROR_CORRUPT_OBJECT;

		sec = params[2].memref.buffer;
		if (!*sec)
			return TEE_SUCCESS;

		return stm32_rproc_enable_sec_boot(params[0].value.a);

	default:
		break;
	}

	return TEE_ERROR_NOT_IMPLEMENTED;
}

static TEE_Result rproc_pta_release_resources(uint32_t pt,
					      TEE_Param params[TEE_NUM_PARAMS])
{
	const uint32_t exp_pt = TEE_PARAM_TYPES(TEE_PARAM_TYPE_VALUE_INPUT,
						TEE_PARAM_TYPE_NONE,
						TEE_PARAM_TYPE_NONE,
						TEE_PARAM_TYPE_NONE);

	if (pt != exp_pt)
		return TEE_ERROR_BAD_PARAMETERS;

	if (rproc_ta_state != REMOTEPROC_OFF)
		return TEE_ERROR_BAD_STATE;

	/* Clean the resources */
	return stm32_rproc_clean_up_memories(params[0].value.a);
}

static TEE_Result rproc_pta_get_mem(uint32_t pt,
				    TEE_Param params[TEE_NUM_PARAMS])
{
	const uint32_t exp_pt = TEE_PARAM_TYPES(TEE_PARAM_TYPE_VALUE_INPUT,
						TEE_PARAM_TYPE_NONE,
						TEE_PARAM_TYPE_NONE,
						TEE_PARAM_TYPE_NONE);

	if (pt != exp_pt)
		return TEE_ERROR_BAD_PARAMETERS;

	if (rproc_ta_state != REMOTEPROC_OFF)
		return TEE_ERROR_BAD_STATE;

	return stm32_rproc_get_mem(params[0].value.a);
}

static TEE_Result rproc_pta_release_mem(uint32_t pt,
					TEE_Param params[TEE_NUM_PARAMS])
{
	const uint32_t exp_pt = TEE_PARAM_TYPES(TEE_PARAM_TYPE_VALUE_INPUT,
						TEE_PARAM_TYPE_NONE,
						TEE_PARAM_TYPE_NONE,
						TEE_PARAM_TYPE_NONE);

	if (pt != exp_pt)
		return TEE_ERROR_BAD_PARAMETERS;

	if (rproc_ta_state != REMOTEPROC_OFF)
		return TEE_ERROR_BAD_STATE;

	return stm32_rproc_release_mem(params[0].value.a);
}

static TEE_Result rproc_pta_invoke_command(void *session __unused,
					   uint32_t cmd_id,
					   uint32_t param_types,
					   TEE_Param params[TEE_NUM_PARAMS])
{
	switch (cmd_id) {
	case PTA_RPROC_HW_CAPABILITIES:
		return rproc_pta_capabilities(param_types, params);
	case PTA_RPROC_LOAD_SEGMENT:
		return rproc_pta_load_segment(param_types, params);
	case PTA_RPROC_SET_MEMORY:
		return rproc_pta_set_memory(param_types, params);
	case PTA_RPROC_FIRMWARE_START:
		return rproc_pta_start(param_types, params);
	case PTA_RPROC_FIRMWARE_STOP:
		return rproc_pta_stop(param_types, params);
	case PTA_RPROC_FIRMWARE_DA_TO_PA:
		return rproc_pta_da_to_pa(param_types, params);
	case PTA_RPROC_VERIFY_DIGEST:
		return rproc_pta_verify_digest(param_types, params);
	case PTA_REMOTEPROC_TLV_PARAM:
		return rproc_pta_tlv_param(param_types, params);
	case PTA_REMOTEPROC_RELEASE:
		return rproc_pta_release_resources(param_types, params);
	case PTA_REMOTEPROC_GET_MEM:
		return rproc_pta_get_mem(param_types, params);
	case PTA_REMOTEPROC_RELEASE_MEM:
		return rproc_pta_release_mem(param_types, params);
	default:
		return TEE_ERROR_NOT_IMPLEMENTED;
	}
}

/*
 * Pseudo Trusted Application entry points
 */
static TEE_Result rproc_pta_open_session(uint32_t pt,
					 TEE_Param params[TEE_NUM_PARAMS],
					 void **sess_ctx __unused)
{
	struct ts_session *s = ts_get_calling_session();
	const uint32_t exp_pt = TEE_PARAM_TYPES(TEE_PARAM_TYPE_VALUE_INPUT,
						TEE_PARAM_TYPE_NONE,
						TEE_PARAM_TYPE_NONE,
						TEE_PARAM_TYPE_NONE);
	struct ts_ctx *ctx = NULL;
	TEE_UUID ta_uuid = TA_REMOTEPROC_UUID;

	if (pt != exp_pt)
		return TEE_ERROR_BAD_PARAMETERS;

	if (!s || !is_user_ta_ctx(s->ctx))
		return TEE_ERROR_ACCESS_DENIED;

	/* Check that we're called by the remoteproc Trusted application*/
	ctx = s->ctx;
	if (memcmp(&ctx->uuid, &ta_uuid, sizeof(TEE_UUID)))
		return TEE_ERROR_ACCESS_DENIED;

	if (!stm32_rproc_get(params[0].value.a))
		return TEE_ERROR_NOT_SUPPORTED;

	return TEE_SUCCESS;
}

pseudo_ta_register(.uuid = PTA_RPROC_UUID, .name = PTA_NAME,
		   .flags = PTA_DEFAULT_FLAGS,
		   .invoke_command_entry_point = rproc_pta_invoke_command,
		   .open_session_entry_point = rproc_pta_open_session);
