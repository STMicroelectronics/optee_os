/*
 * Copyright (c) 2020 Arm Limited. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef PSA_ADAC_PSA_CRYPTO_API_H
#define PSA_ADAC_PSA_CRYPTO_API_H

#include <psa/crypto.h>

/** \addtogroup adac-crypto-api
 * @{
 */

/** \brief ADAC cryptographic back-end initialization
 *
 * This function will be called by ADAC library.
 */
psa_status_t psa_adac_crypto_init(void);

/** \brief Generate challenge
 *
 * \param[out] output       Output buffer for the challenge.
 * \param output_size       Number of bytes to generate and output.
 */
psa_status_t psa_adac_generate_challenge(uint8_t *output, size_t output_size);

/** \brief Compute the hash of a message
 *
 * \param alg               The hash algorithm to compute.
 * \param[in] input         Buffer containing the message to hash.
 * \param input_size        Size of the \p input buffer in bytes.
 * \param[out] hash         Buffer where the hash is to be written.
 * \param hash_size         Size of the \p hash buffer in bytes.
 * \param[out] hash_length  On success, the length of the hash in bytes.
 *
 * \retval PSA_SUCCESS
 *         Success.
 * \retval PSA_ERROR_NOT_SUPPORTED
 *         \p alg is not supported (or unknown hash algorithm).
 * \retval PSA_ERROR_INVALID_ARGUMENT
 * \retval PSA_ERROR_HARDWARE_FAILURE
 */
psa_status_t psa_adac_hash(psa_algorithm_t alg, const uint8_t *input, size_t input_size,
                           uint8_t *hash, size_t hash_size, size_t *hash_length);

/** \brief Compute the hash of a message composed of multiple parts
 *
 * \param alg               The hash algorithm to compute.
 * \param[in] inputs        Array of buffers containing the message to hash.
 * \param[in] input_sizes   Array of size of the \p inputs buffers in bytes.
 * \param input_count       Number of entries in \p inputs and \p input_sizes.
 * \param[out] hash         Buffer where the hash is to be written.
 * \param hash_size         Size of the \p hash buffer in bytes.
 * \param[out] hash_length  On success, the length of the hash in bytes.
 *
 * \retval PSA_SUCCESS
 *         Success.
 * \retval PSA_ERROR_NOT_SUPPORTED
 *         \p alg is not supported (or unknown hash algorithm).
 * \retval PSA_ERROR_INVALID_ARGUMENT
 * \retval PSA_ERROR_HARDWARE_FAILURE
 */
psa_status_t psa_adac_hash_multiple(psa_algorithm_t alg,
                                    const uint8_t *inputs[], size_t input_sizes[], size_t input_count,
                                    uint8_t hash[], size_t hash_size, size_t *hash_length);

/** \brief Compute the hash of a message and compare it with an expected value.
 *
 * \param alg               The hash algorithm to compute.
 * \param[in] input         Buffer containing the message to hash.
 * \param input_size        Size of the \p input buffer in bytes.
 * \param[out] hash         Buffer containing the expected hash value.
 * \param hash_size         Size of the \p hash buffer in bytes.
 *
 * \retval PSA_SUCCESS
 *         The expected hash is identical to the actual hash of the input.
 * \retval PSA_ERROR_INVALID_SIGNATURE
 *         The hash of the message was calculated successfully, but it
 *         differs from the expected hash.
 * \retval PSA_ERROR_NOT_SUPPORTED
 *         \p alg is not supported (or unknown hash algorithm).
 * \retval PSA_ERROR_INVALID_ARGUMENT
 * \retval PSA_ERROR_HARDWARE_FAILURE
 */
psa_status_t psa_adac_hash_verify(psa_algorithm_t alg, const uint8_t input[], size_t input_size,
                                  uint8_t hash[], size_t hash_size);

/** \brief Compute the hash of a message and compare it with a list of expected values
 *
 */
psa_status_t psa_adac_hash_verify_multiple(psa_algorithm_t alg, const uint8_t input[], size_t input_length,
                                           uint8_t *hash[], size_t hash_size[], size_t hash_count);

/** \brief Verify a signature
 *
 */
psa_status_t psa_adac_verify_signature(uint8_t key_type, uint8_t *key, size_t key_size, psa_algorithm_t hash_algo,
                                       const uint8_t *inputs[], size_t input_sizes[], size_t input_count,
                                       psa_algorithm_t sig_algo, uint8_t *sig, size_t sig_size);

/** \brief Verify a message authentication code
 *
 */
psa_status_t psa_adac_mac_verify(psa_algorithm_t alg, const uint8_t *inputs[], size_t input_sizes[], size_t input_count,
                                 const uint8_t key[], size_t key_size, uint8_t mac[], size_t mac_size);

/** \brief Derive key
 *
 */
psa_status_t psa_adac_derive_key(uint8_t *crt, size_t crt_size, uint8_t key_type, uint8_t *key, size_t key_size);

/**@}*/

#endif //PSA_ADAC_PSA_CRYPTO_API_H
