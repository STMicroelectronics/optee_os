// SPDX-License-Identifier: BSD-3-Clause
/*
 * Copyright (c) 2020-2025 Arm Limited. All rights reserved.
 * Copyright (c) 2025 STMicroelectronics - All Rights Reserved
 */

#include <crypto/crypto.h>
#include <psa/crypto.h>
#include <psa_adac.h>
#include <psa_adac_config.h>
#include <psa_adac_crypto_api.h>
#include <psa_adac_cryptosystems.h>
#include <psa_adac_debug.h>
#include <string_ext.h>
#include <tee_api_types.h>
#include <utee_defines.h>

/**
 * psa_adac_crypto_init() - ADAC cryptographic back-end initialization
 *
 * This function will be called by ADAC library.
 *
 * Returns PSA_SUCCESS as cryptographic back-end is already initialized.
 */
psa_status_t psa_adac_crypto_init(void)
{
	return PSA_SUCCESS;
}

/**
 * psa_adac_generate_challenge() - Generate challenge
 *
 * This function will be called by ADAC library.
 *
 * @output: [out] Output buffer for the challenge.
 * @output_size: Number of bytes to generate and output.
 *
 * Returns PSA_SUCCESS if challenge generation succeeded,
 * a psa_status_t error value otherwise.
 */
psa_status_t psa_adac_generate_challenge(uint8_t *output, size_t output_size)
{
	TEE_Result res = TEE_ERROR_GENERIC;

	res = crypto_rng_read(output, output_size);
	if (res) {
		EMSG("Failed to generate challenge with RNG: %#" PRIx32, res);
		return PSA_ERROR_NOT_SUPPORTED;
	}

	return PSA_SUCCESS;
}

/**
 * psa_adac_hash() - Compute the hash of a message
 *
 * This function will be called by ADAC library.
 *
 * @alg: The hash algorithm to compute.
 * @input: [in] Buffer containing the message to hash.
 * @input_size: Size of the input buffer in bytes.
 * @hash: [out] Buffer where the hash is to be written.
 * @hash_size: Size of the hash buffer in bytes.
 * @hash_length: [out] On success, the length of the hash in bytes.
 *
 * Returns PSA_SUCCESS, if hash computation went well, PSA_ERROR_NOT_SUPPORTED
 * if hash algorithm is not supported or unknown, PSA_ERROR_HARDWARE_FAILURE
 * otherwise.
 */
psa_status_t psa_adac_hash(psa_algorithm_t alg, const uint8_t *input,
			   size_t input_size, uint8_t *hash, size_t hash_size,
			   size_t *hash_length)
{
	return psa_adac_hash_multiple(alg, &input, &input_size, 1, hash,
				      hash_size, hash_length);
}

/**
 * psa_adac_hash_multiple() - Compute the hash of a message composed of multiple
 *			      parts
 *
 * @alg: The hash algorithm to compute.
 * @inputs: [in] Array of buffers containing the message to hash.
 * @input_sizes: [in] Array of size of the inputs buffers in bytes.
 * @input_count: Number of entries in inputs and input_sizes.
 * @hash: [out] Buffer where the hash is to be written.
 * @hash_size: Size of the hash buffer in bytes.
 * @hash_length: [out] On success, the length of the hash in bytes.
 *
 * Returns PSA_SUCCESS, if hash computation went well, PSA_ERROR_NOT_SUPPORTED
 * if hash algorithm is not supported or unknown, PSA_ERROR_HARDWARE_FAILURE
 * otherwise.
 */
psa_status_t psa_adac_hash_multiple(psa_algorithm_t alg,
				    const uint8_t *inputs[],
				    size_t input_sizes[], size_t input_count,
				    uint8_t hash[], size_t hash_size,
				    size_t *hash_length)
{
	TEE_Result res = TEE_ERROR_GENERIC;
	void *ctx = NULL;
	uint32_t i = 0;

	if (alg != PSA_ALG_SHA_256 || hash_size < TEE_SHA256_HASH_SIZE) {
		EMSG("Only SHA2-256 is supported, alg: %#" PRIx32, alg);
		return PSA_ERROR_NOT_SUPPORTED;
	}

	res = crypto_hash_alloc_ctx(&ctx, TEE_ALG_SHA256);
	if (res)
		goto out;

	res = crypto_hash_init(ctx);
	if (res)
		goto out_free_ctx;

	for (i = 0; i < input_count; i++) {
		res = crypto_hash_update(ctx, inputs[i], input_sizes[i]);
		if (res)
			goto out_free_ctx;
	}

	*hash_length = TEE_SHA256_HASH_SIZE;
	res = crypto_hash_final(ctx, hash, *hash_length);

out_free_ctx:
	crypto_hash_free_ctx(ctx);
out:
	if (res) {
		EMSG("Failed to compute hash: %#" PRIx32, res);
		return PSA_ERROR_HARDWARE_FAILURE;
	}

	return PSA_SUCCESS;
}

/**
 * hash_check() - Compare first hash value with second hash value.
 *
 * @input_a: [in] Buffer containing the first hash value.
 * @len_a: Size of the input_a buffer in bytes.
 * @input_b: [in] Buffer containing the second hash value.
 * @len_b: Size of the input_b buffer in bytes.
 *
 * Returns PSA_SUCCESS if the first hash is identical to the second hash,
 * PSA_ERROR_INVALID_SIGNATURE otherwise.
 */
static psa_status_t hash_check(const uint8_t *input_a, size_t len_a,
			       const uint8_t *input_b, size_t len_b)
{
	int32_t result = 1;

	if (len_a == len_b)
		result = consttime_memcmp(input_b, input_a, len_a);

	return (result == 0U) ? PSA_SUCCESS : PSA_ERROR_INVALID_SIGNATURE;
}

/**
 * psa_adac_hash_verify() - Compute the hash of a message and compare it with an
 *			    expected value.
 *
 * @alg: The hash algorithm to compute.
 * @input: [in] Buffer containing the message to hash.
 * @input_size: Size of the input buffer in bytes.
 * @hash: [out]Buffer containing the expected hash value.
 * @hash_size: Size of the hash buffer in bytes.
 *
 * Return PSA_SUCCESS if the expected hash is identical to the actual hash of
 * the input, PSA_ERROR_INVALID_SIGNATURE if the hash of the message was
 * calculated successfully, but it differs from the expected hash, or
 * PSA_ERROR_NOT_SUPPORTED if alg is not supported (or unknown hash algorithm).
 */
psa_status_t psa_adac_hash_verify(psa_algorithm_t alg, const uint8_t input[],
				  size_t input_size, uint8_t hash[],
				  size_t hash_size)
{
	return psa_adac_hash_verify_multiple(alg, input, input_size, &hash,
					     &hash_size, 1);
}

/**
 * psa_adac_hash_verify_multiple() - Compute the hash of a message and compare
 *				     it with a list of expected values
 * @alg: The hash algorithm to compute.
 * @input: [in] Buffer containing the message to hash.
 * @input_length: Size of the input buffer in bytes.
 * @hash: [out] Array of buffers containing the expected hash values.
 * @hash_size: Array of sizes of the hash buffers in bytes.
 * @hash_count: Number of entries in hash and hash_size.
 *
 * Return PSA_SUCCESS if the expected hash is identical to the actual hash of
 * the input, PSA_ERROR_INVALID_SIGNATURE if the hash of the message was
 * calculated successfully, but it differs from the expected hash, or
 * PSA_ERROR_NOT_SUPPORTED if alg is not supported (or unknown hash algorithm).
 */
psa_status_t psa_adac_hash_verify_multiple(psa_algorithm_t alg,
					   const uint8_t input[],
					   size_t input_length, uint8_t *hash[],
					   size_t hash_size[],
					   size_t hash_count)
{
	psa_status_t r = PSA_ERROR_GENERIC_ERROR;
	uint8_t output[TEE_SHA256_HASH_SIZE];
	size_t output_size = 0;

	r = psa_adac_hash_multiple(alg, &input, &input_length, 1, output,
				   sizeof(output), &output_size);

	if (r == PSA_SUCCESS) {
		for (size_t i = 0; i < hash_count; i++) {
			r = hash_check(hash[i], hash_size[i], output,
				       output_size);
			if (r == PSA_SUCCESS)
				return r;
		}
	}

	return r;
}

/**
 * psa_adac_verify_signature() - Verify a signature
 *
 * @key_type: Type of the Public key used to verify the signature.
 * @key: [in] Buffer containing the Public key used to verify the signature.
 * @key_size: [in] Size of the key buffer in bytes.
 * @hash_algo: The hash algorithm to compute.
 * @inputs: [in] Array of buffers containing the signed message to verify.
 * @input_sizes: [in] Array of size of the inputs buffers in bytes.
 * @input_count: Number of entries in inputs and input_sizes.
 * @sig_algo: The algorithm used to generate the signature.
 * @sig: Buffer containing the signature.
 * @sig_size: Size of the sig buffer in bytes.
 *
 * Returns PSA_SUCCESS, if the message is authenticated, PSA_ERROR_NOT_SUPPORTED
 * if the Public key type or size, or the signature algorithm or size, or the
 * hash algorithm are not supported or unknown, or PSA_ERROR_INVALID_SIGNATURE
 * if the authentication is failed.
 */
psa_status_t psa_adac_verify_signature(uint8_t key_type, uint8_t *key,
				       size_t key_size,
				       psa_algorithm_t hash_algo,
				       const uint8_t *inputs[],
				       size_t input_sizes[], size_t input_count,
				       psa_algorithm_t sig_algo, uint8_t *sig,
				       size_t sig_size)
{
	psa_status_t r = PSA_ERROR_GENERIC_ERROR;
	TEE_Result res = TEE_ERROR_GENERIC;
	struct ecc_public_key pubkey = {};
	uint8_t hash[TEE_SHA256_HASH_SIZE];
	size_t coord_len = sig_size / 2;
	size_t hash_length = 0;

	if (key_type != ECDSA_P256_SHA256 ||
	    key_size != ECDSA_P256_PUBLIC_KEY_SIZE ||
	    sig_algo != ECDSA_P256_SIGN_ALGORITHM ||
	    sig_size != ECDSA_P256_SIGNATURE_SIZE ||
	    hash_algo != ECDSA_P256_HASH_ALGORITHM) {
		EMSG("Only ECDSA P256 is supported");
		return PSA_ERROR_NOT_SUPPORTED;
	}

	r = psa_adac_hash_multiple(hash_algo, inputs, input_sizes, input_count,
				   hash, sizeof(hash), &hash_length);
	if (r != PSA_SUCCESS)
		return r;

	res = crypto_acipher_alloc_ecc_public_key(&pubkey,
						  TEE_TYPE_ECDSA_PUBLIC_KEY,
						  key_size);
	if (res) {
		EMSG("Public key allocation failure: %#" PRIx32, res);
		return PSA_ERROR_HARDWARE_FAILURE;
	}

	pubkey.curve = TEE_ECC_CURVE_NIST_P256;
	res = crypto_bignum_bin2bn(key, coord_len, pubkey.x);
	if (res)
		goto out_free_key;
	res = crypto_bignum_bin2bn(key + coord_len, coord_len, pubkey.y);
	if (res)
		goto out_free_key;

	res = crypto_acipher_ecc_verify(TEE_ALG_ECDSA_P256, &pubkey, hash,
					hash_length, sig, sig_size);

out_free_key:
	crypto_acipher_free_ecc_public_key(&pubkey);

	if (res) {
		EMSG("Message authentication failure: %#" PRIx32, res);
		return PSA_ERROR_INVALID_SIGNATURE;
	}

	return PSA_SUCCESS;
}

#if defined(PSA_ADAC_CMAC) || defined(PSA_ADAC_HMAC)
psa_status_t psa_adac_mac_verify(psa_algorithm_t alg, const uint8_t *inputs[],
				 size_t input_sizes[], size_t input_count,
				 const uint8_t key[], size_t key_size,
				 uint8_t mac[], size_t mac_size)
{
	return PSA_ERROR_NOT_SUPPORTED;
}

psa_status_t psa_adac_derive_key(uint8_t *crt, size_t crt_size,
				 uint8_t key_type, uint8_t *key,
				 size_t key_size)
{
	return PSA_ERROR_NOT_SUPPORTED;
}
#endif
