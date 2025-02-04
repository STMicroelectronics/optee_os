/* SPDX-License-Identifier: BSD-3-Clause */
/*
 * Copyright (c) 2018-2022, Arm Limited. All rights reserved.
 * Copyright (c) 2025, STMicroelectronics
 *
 */

#ifndef __TFM_CRYPTO_DEFS_H__
#define __TFM_CRYPTO_DEFS_H__

#ifdef __cplusplus
extern "C" {
#endif

#include "psa/crypto.h"
#ifdef PLATFORM_DEFAULT_CRYPTO_KEYS
#include "crypto_keys/tfm_builtin_key_ids.h"
#else
#include "tfm_builtin_key_ids.h"
#endif /* PLATFORM_DEFAULT_CRYPTO_KEYS */

/**
 * \brief The maximum supported length of a nonce through the TF-M
 *        interfaces
 */
#define TFM_CRYPTO_MAX_NONCE_LENGTH (16u)

/**
 * \brief This type is used to overcome a limitation in the number of maximum
 *        IOVECs that can be used especially in psa_aead_encrypt and
 *        psa_aead_decrypt. By using this type we pack the nonce and the actual
 *        nonce_length at part of the same structure
 *
 */
struct tfm_crypto_aead_pack_input {
	uint8_t nonce[TFM_CRYPTO_MAX_NONCE_LENGTH];
	uint32_t nonce_length;
};

/**
 * \brief Structure used to pack non-pointer types in a call to PSA Crypto APIs
 *
 */
struct tfm_crypto_pack_iovec {
	psa_key_id_t key_id;     /*!< Key id */
	psa_algorithm_t alg;     /*!< Algorithm */
	uint32_t op_handle;      /*!< Client context handle associated to a
				  *   multipart operation
				  */
	uint32_t ad_length;        /*!< Additional Data length for multipart AEAD */
	uint32_t plaintext_length; /*!< Plaintext length for multipart AEAD */

	struct tfm_crypto_aead_pack_input aead_in; /*!< Packs AEAD-related inputs */

	uint16_t function_id;    /*!< Used to identify the function in the
				  *   API dispatcher to the service backend
				  *   See tfm_crypto_func_SID, for detail
				  */
	uint16_t step;           /*!< Key derivation step */
	union {
		uint32_t capacity;     /*!< Key derivation capacity */
		uint64_t value;      /*!< Key derivation integer for update*/
	};
};

/**
 * \brief Type associated to the group of a function encoding. There can be
 *        nine groups (Random, Key management, Hash, MAC, Cipher, AEAD,
 *        Asym sign, Asym encrypt, Key derivation).
 */
enum tfm_crypto_group_id_t {
	TFM_CRYPTO_GROUP_ID_RANDOM          = UINT8_C(1),
	TFM_CRYPTO_GROUP_ID_KEY_MANAGEMENT  = UINT8_C(2),
	TFM_CRYPTO_GROUP_ID_HASH            = UINT8_C(3),
	TFM_CRYPTO_GROUP_ID_MAC             = UINT8_C(4),
	TFM_CRYPTO_GROUP_ID_CIPHER          = UINT8_C(5),
	TFM_CRYPTO_GROUP_ID_AEAD            = UINT8_C(6),
	TFM_CRYPTO_GROUP_ID_ASYM_SIGN       = UINT8_C(7),
	TFM_CRYPTO_GROUP_ID_ASYM_ENCRYPT    = UINT8_C(8),
	TFM_CRYPTO_GROUP_ID_KEY_DERIVATION  = UINT8_C(9)
};

#define BVAL(x) (((uint16_t)((((uint16_t)(x)) << 8) & 0xFF00)) - 1)

/**
 * \brief This type defines numerical progressive values
 * identifying a function API exposed through the interfaces
 * (S or NS). It's used to dispatch the requests
 * from S/NS to the corresponding API implementation in
 * the Crypto service backend.
 *
 * \note Each function SID is encoded as uint16_t.
 *        +------------+------------+
 *        |  Group ID  |  Func ID   |
 *        +------------+------------+
 *   (MSB)15         8 7          0(LSB)
 *
 */
enum tfm_crypto_func_sid_t {
	BASE__RANDOM         = BVAL(TFM_CRYPTO_GROUP_ID_RANDOM),
	TFM_CRYPTO_GENERATE_RANDOM_SID,
	BASE__KEY_MANAGEMENT = BVAL(TFM_CRYPTO_GROUP_ID_KEY_MANAGEMENT),
	TFM_CRYPTO_GET_KEY_ATTRIBUTES_SID,
	TFM_CRYPTO_OPEN_KEY_SID,
	TFM_CRYPTO_CLOSE_KEY_SID,
	TFM_CRYPTO_IMPORT_KEY_SID,
	TFM_CRYPTO_DESTROY_KEY_SID,
	TFM_CRYPTO_EXPORT_KEY_SID,
	TFM_CRYPTO_EXPORT_PUBLIC_KEY_SID,
	TFM_CRYPTO_PURGE_KEY_SID,
	TFM_CRYPTO_COPY_KEY_SID,
	TFM_CRYPTO_GENERATE_KEY_SID,
	BASE__HASH           = BVAL(TFM_CRYPTO_GROUP_ID_HASH),
	TFM_CRYPTO_HASH_COMPUTE_SID,
	TFM_CRYPTO_HASH_COMPARE_SID,
	TFM_CRYPTO_HASH_SETUP_SID,
	TFM_CRYPTO_HASH_UPDATE_SID,
	TFM_CRYPTO_HASH_CLONE_SID,
	TFM_CRYPTO_HASH_FINISH_SID,
	TFM_CRYPTO_HASH_VERIFY_SID,
	TFM_CRYPTO_HASH_ABORT_SID,
	BASE__MAC            = BVAL(TFM_CRYPTO_GROUP_ID_MAC),
	TFM_CRYPTO_MAC_COMPUTE_SID,
	TFM_CRYPTO_MAC_VERIFY_SID,
	TFM_CRYPTO_MAC_SIGN_SETUP_SID,
	TFM_CRYPTO_MAC_VERIFY_SETUP_SID,
	TFM_CRYPTO_MAC_UPDATE_SID,
	TFM_CRYPTO_MAC_SIGN_FINISH_SID,
	TFM_CRYPTO_MAC_VERIFY_FINISH_SID,
	TFM_CRYPTO_MAC_ABORT_SID,
	BASE__CIPHER         = BVAL(TFM_CRYPTO_GROUP_ID_CIPHER),
	TFM_CRYPTO_CIPHER_ENCRYPT_SID,
	TFM_CRYPTO_CIPHER_DECRYPT_SID,
	TFM_CRYPTO_CIPHER_ENCRYPT_SETUP_SID,
	TFM_CRYPTO_CIPHER_DECRYPT_SETUP_SID,
	TFM_CRYPTO_CIPHER_GENERATE_IV_SID,
	TFM_CRYPTO_CIPHER_SET_IV_SID,
	TFM_CRYPTO_CIPHER_UPDATE_SID,
	TFM_CRYPTO_CIPHER_FINISH_SID,
	TFM_CRYPTO_CIPHER_ABORT_SID,
	BASE__AEAD           = BVAL(TFM_CRYPTO_GROUP_ID_AEAD),
	TFM_CRYPTO_AEAD_ENCRYPT_SID,
	TFM_CRYPTO_AEAD_DECRYPT_SID,
	TFM_CRYPTO_AEAD_ENCRYPT_SETUP_SID,
	TFM_CRYPTO_AEAD_DECRYPT_SETUP_SID,
	TFM_CRYPTO_AEAD_GENERATE_NONCE_SID,
	TFM_CRYPTO_AEAD_SET_NONCE_SID,
	TFM_CRYPTO_AEAD_SET_LENGTHS_SID,
	TFM_CRYPTO_AEAD_UPDATE_AD_SID,
	TFM_CRYPTO_AEAD_UPDATE_SID,
	TFM_CRYPTO_AEAD_FINISH_SID,
	TFM_CRYPTO_AEAD_VERIFY_SID,
	TFM_CRYPTO_AEAD_ABORT_SID,
	BASE__ASYM_SIGN      = BVAL(TFM_CRYPTO_GROUP_ID_ASYM_SIGN),
	TFM_CRYPTO_ASYMMETRIC_SIGN_MESSAGE_SID,
	TFM_CRYPTO_ASYMMETRIC_VERIFY_MESSAGE_SID,
	TFM_CRYPTO_ASYMMETRIC_SIGN_HASH_SID,
	TFM_CRYPTO_ASYMMETRIC_VERIFY_HASH_SID,
	BASE__ASYM_ENCRYPT   = BVAL(TFM_CRYPTO_GROUP_ID_ASYM_ENCRYPT),
	TFM_CRYPTO_ASYMMETRIC_ENCRYPT_SID,
	TFM_CRYPTO_ASYMMETRIC_DECRYPT_SID,
	BASE__KEY_DERIVATION = BVAL(TFM_CRYPTO_GROUP_ID_KEY_DERIVATION),
	TFM_CRYPTO_RAW_KEY_AGREEMENT_SID,
	TFM_CRYPTO_KEY_DERIVATION_SETUP_SID,
	TFM_CRYPTO_KEY_DERIVATION_GET_CAPACITY_SID,
	TFM_CRYPTO_KEY_DERIVATION_SET_CAPACITY_SID,
	TFM_CRYPTO_KEY_DERIVATION_INPUT_BYTES_SID,
	TFM_CRYPTO_KEY_DERIVATION_INPUT_KEY_SID,
	TFM_CRYPTO_KEY_DERIVATION_INPUT_INTEGER_SID,
	TFM_CRYPTO_KEY_DERIVATION_KEY_AGREEMENT_SID,
	TFM_CRYPTO_KEY_DERIVATION_OUTPUT_BYTES_SID,
	TFM_CRYPTO_KEY_DERIVATION_OUTPUT_KEY_SID,
	TFM_CRYPTO_KEY_DERIVATION_ABORT_SID,
};

/**
 * \brief This macro is used to extract the group_id from an encoded function id
 *        by accessing the upper 8 bits. A \a _function_id is uint16_t type
 */
#define TFM_CRYPTO_GET_GROUP_ID(_function_id) \
	((enum tfm_crypto_group_id_t)(((uint16_t)(_function_id) >> 8) & 0xFF))

#ifdef __cplusplus
}
#endif

#endif /* __TFM_CRYPTO_DEFS_H__ */
