/*
 * Copyright (c) 2022-2023, Arm Limited. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 */

#ifndef __PSA_ADAC_SDM_H__
#define __PSA_ADAC_SDM_H__

#if defined(MBEDTLS_CONFIG_FILE)
#include MBEDTLS_CONFIG_FILE
#endif

#include "psa_adac.h"

#if defined(MBEDTLS_FS_IO)


#ifdef __cplusplus
extern "C" {
#endif

/** \addtogroup adac-sdm
 * @{
 */

/**
 * \brief Parses the input trust chain data into type length value format
 *
 * \param[in]  input      Pointer to input data buffer.
 * \param[in]  input_size Size of input buffer in bytes.
 * \param[out] tlvs       Double pointer to the output tlv data.
 * \param[in]  max_tlvs   Maximum number of allowed extensions.
 * \param[out] tlv_count  Pointer to number of extensions in the chain.
 *
 * \return 0 on success, non-zero otherwise
 *
 */
int split_tlv_static(uint32_t *input, size_t input_size, psa_tlv_t **tlvs,
                     size_t max_tlvs, size_t *tlv_count);

/**
 * \brief Split input chain data to individual extensions
 *
 * \param[in]  input           Pointer to the input chain data.
 * \param[in]  input_size      Size of input buffer.
 * \param[out] extensions      Double pointer to output extensions.
 * \param[out] extension_count Pointer to the number of extensions.
 *
 * \return 0 on success, non-zero otherwise
 *
 */
int split_extensions(uint32_t *input, size_t input_size, psa_tlv_t **extensions[],
                     size_t *extension_count);

/**
 * \brief Loads the key from the input key file
 *
 * \param[in]  key_file Pointer to input key file path.
 * \param[out] type     Pointer to the output key type.
 * \param[out] handle   Pointer to output key id.
 *
 * \return 0 on success, non-zero otherwise
 *
 */
int import_private_key(const char *key_file, uint8_t *type, psa_key_handle_t *handle);

/**
 * \brief Loads the secret key from input file (for symmetric key authentication)
 *
 * \param[in]  key_file Pointer to input key file path.
 * \param[in]  type     Input key type.
 * \param[out] key      Double pointer to output secret key.
 * \param[out] key_size Pointer to size of output key in bytes.
 *
 * \return 0 on success, non-zero otherwise
 *
 */
int load_secret_key(const char *key_file, uint8_t type, uint8_t **key, size_t *key_size);

/**
 * \brief Loads the chain of trust.
 *        An example trust chain may be composed of leaf certificate, zero or more intermediary
 *        certificates, and ends in a root certificate.
 *
 * \param[in]  chain_file Pointer to chain file path.
 * \param[out] chain      Double pointer to the decoded base64 formatted input chain data buffer.
 * \param[out] chain_size Pointer to the size of the chain.
 *
 * \return 0 on success, non-zero otherwise
 *
 */
int load_trust_chain(const char *chain_file, uint8_t **chain, size_t *chain_size);

/**
 * \brief Loads root of trust public key.
 *
 * \param[in]  chain_file  Pointer to chain file path.
 * \param[in]  alg         Input hash algorithm type.
 * \param[out] rotpk       Pointer to output public key data buffer.
 * \param[in]  buffer_size Size of \p rotpk buffer in bytes.
 * \param[out] rotpk_size  Pointer to length of the output data buffer in bytes.
 * \param[out] rotpk_type  Pointer to root of trust public key type.
 *
 * \return 0 on success, non-zero otherwise
 *
 */
int load_trust_rotpk(const char *chain_file, psa_algorithm_t alg, uint8_t *rotpk,
                     size_t buffer_size, size_t *rotpk_size, uint8_t *rotpk_type);

/**
 * \brief Signs the debug token for authentication
 *
 * \param[in]  challenge      Pointer to input challenge.
 * \param[in]  challenge_size Size of input challenge data.
 * \param[in]  signature_type Algorithm type used for signature .
 * \param[in]  exts           Pointer to input extension data.
 * \param[in]  exts_size      Size of extension data buffer.
 * \param[out] fragment       Double pointer to the output response fragment.
 * \param[out] fragment_size  Pointer to size of response fragment.
 * \param[in]  req_perms      Pointer to the requested permissions data.
 * \param[in]  handle         Key handle (for asymmetric key signature).
 * \param[in]  key            Pointer to key to be used for signature (for symmetric key signature).
 * \param[in]  key_size       Size of input key data buffer.
 *
 * \return A status indicating the success/failure of the operation as specified
 *         in \ref psa_status_t
 *
 */
psa_status_t psa_adac_sign_token(uint8_t challenge[], size_t challenge_size, uint8_t signature_type,
                                 uint8_t exts[], size_t exts_size, uint8_t *fragment[],
                                 size_t *fragment_size, uint8_t req_perms[],
                                 psa_key_handle_t handle, uint8_t *key, size_t key_size);

/**@}*/

#ifdef __cplusplus
}
#endif

#endif /* #if defined(MBEDTLS_FS_IO) */

#endif /* __PSA_ADAC_SDM_H__ */
