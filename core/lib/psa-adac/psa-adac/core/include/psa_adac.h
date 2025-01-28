/*
 * Copyright (c) 2020-2023, Arm Limited. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 */

/**
 * \file
 * \brief ADAC Core
 */

#ifndef __PSA_ADAC_H__
#define __PSA_ADAC_H__

#include <stdint.h>
#include "psa/crypto.h"

#ifdef __cplusplus
extern "C" {
#endif

/** \addtogroup adac-core
 * @{
 */

#define ROUND_TO_WORD(x) (((size_t)x + 3) & ~0x03UL)

/** \brief Version
 *
 * Current version numbers for certificate and token format.
 */
#define ADAC_CERT_MAJOR 1u
#define ADAC_CERT_MINOR 0u
#define ADAC_TOKEN_MAJOR 1u
#define ADAC_TOKEN_MINOR 0u

/** \brief Key options
 *
 */
typedef enum {
    ECDSA_P256_SHA256 = 0x01, /**< EC key using P-256 curve, ECDSA signature with SHA-256 */
    ECDSA_P521_SHA512 = 0x02, /**< EC key using P-521 curve, ECDSA signature with SHA-512 */
    RSA_3072_SHA256 = 0x03,   /**< 3072-bit RSA key, RSA signature with SHA-256 */
    RSA_4096_SHA256 = 0x04,   /**< 4096-bit RSA key, RSA signature with SHA-256 */
    ED_25519_SHA512 = 0x05,   /**< EC key using Curve25519, EdDSA signature with SHA-512 */
    ED_448_SHAKE256 = 0x06,   /**< EC key using Curve448, EdDSA signature with SHAKE-256 */
    SM_SM2_SM3 = 0x07,        /**< EC key using SM2, ECDSA/SM signature with SM3 */
    CMAC_AES = 0x08,          /**< AES-128 key, CMAC MAC */
    HMAC_SHA256 = 0x09        /**< 256-bit key, HMAC-SHA-256 MAC */
} key_options_t;

/**
 *
 */
typedef enum {
    NULL_TYPE             = 0x0000,
    ADAC_AUTH_VERSION     = 0x0001,
    VENDOR_ID             = 0x0002,
    SOC_CLASS             = 0x0003,
    SOC_ID                = 0x0004,
    TARGET_IDENTITY       = 0x0005,
    HW_PERMISSIONS_FIXED  = 0x0006,
    HW_PERMISSIONS_MASK   = 0x0007,
    PSA_LIFECYCLE         = 0x0008,
    SW_PARTITION_ID       = 0x0009,
    SDA_ID                = 0x000A,
    SDA_VERSION           = 0x000B,
    EFFECTIVE_PERMISSIONS = 0x000C,
    TOKEN_FORMATS         = 0x0100,
    CERT_FORMATS          = 0x0101,
    CRYPTOSYSTEMS         = 0x0102,
    PSA_BINARY_TOKEN      = 0x0200,
    PSA_BINARY_CRT        = 0x0201,
    PSA_X509_CRT          = 0x0202
} type_id_t;

/** \brief Version type
 *
 */
typedef struct {
    uint8_t major;
    uint8_t minor;
} psa_version_t;

/** \brief TLV type for extensions
 *
 */
typedef struct {
    uint16_t _reserved; /* !< Must be set to zero */
    uint16_t type_id;
    uint32_t length_in_bytes;
    uint8_t value[];
} psa_tlv_t;

/** \brief Command protocol request packet
 *
 */
typedef struct {
    uint16_t _reserved; /* !< Must be set to zero */
    uint16_t command;
    uint32_t data_count;
    uint32_t data[];
} request_packet_t;

/** \brief Command protocol response packet
 *
 */
typedef struct {
    uint16_t _reserved; /* !< Must be set to zero */
    uint16_t status;
    uint32_t data_count;
    uint32_t data[];
} response_packet_t;

/** \brief Commands
 *
 */
typedef enum {
    ADAC_DISCOVERY_CMD = 0x01,      /**< `Discovery` command */
    ADAC_AUTH_START_CMD = 0x02,     /**< `Start Authentication` command */
    ADAC_AUTH_RESPONSE_CMD = 0x03,  /**< `Authentication Response` command */
    ADAC_RESUME_BOOT_CMD = 0x05,    /**< `Resume Boot` command */
    ADAC_LOCK_DEBUG_CMD = 0x06,     /**< `Lock Debug` command */
    ADAC_LCS_CHANGE_CMD = 0x07,     /**< `Change Life-cycle State` command */
} adac_commands_t;

/** \brief Status codes
 *
 */
typedef enum {
    ADAC_SUCCESS = 0x0000,
    ADAC_FAILURE = 0x0001,
    ADAC_NEED_MORE_DATA = 0x0002,
    ADAC_UNSUPPORTED = 0x0003,
    ADAC_UNAUTHORIZED = 0x0004,
    ADAC_INVALID_PARAMETERS = 0x0005,
    ADAC_INVALID_COMMAND = 0x7FFF
} adac_status_t;

/** \brief Certification roles
 *
 */
typedef enum {
    ADAC_CRT_ROLE_ROOT = 0x01, /**< Root Certification Authority Certificate */
    ADAC_CRT_ROLE_INT = 0x02,  /**< Intermediate Certification Authority Certificate */
    ADAC_CRT_ROLE_LEAF = 0x03  /**< Leaf Certificate */
} certificate_role_t;

/** \brief Certificate header
 *
 */
typedef struct {
    psa_version_t format_version;
    uint8_t signature_type;
    uint8_t key_type;
    uint8_t role;
    uint8_t usage;
    uint16_t _reserved; /* !< Must be set to zero */
    uint16_t lifecycle;
    uint16_t oem_constraint;
    uint32_t extensions_bytes;
    uint32_t soc_class;
    uint8_t soc_id[16];
    uint8_t permissions_mask[16];
} certificate_header_t;

/** \brief Token header
 *
 */
typedef struct {
    psa_version_t format_version;
    uint8_t signature_type;
    uint8_t _reserved; /* !< Must be set to zero. */
    uint32_t extensions_bytes;
    uint8_t requested_permissions[16];
} token_header_t;

#define CHALLENGE_SIZE 32
#define MAX_EXTENSIONS 16
#define PERMISSION_BITS 128

/** \brief Authentication challenge
 *
 */
typedef struct {
    psa_version_t format_version;
    uint16_t _reserved;
    uint8_t challenge_vector[32];
} psa_auth_challenge_t;

typedef struct {
    uint8_t *content;
    size_t size;
    size_t max;
    uint8_t key_type;
} validation_context_t;

/**
 * \brief ADAC library initialization
 *
 * \return A status indicating the success/failure of the operation as specified
 *         in \ref psa_status_t
 *
 */
psa_status_t psa_adac_init(void);

/**
 * \brief Loads the context from an input key
 *
 * \param[out] context   Pointer to output context.
 * \param[in]  key_type  Type of input key.
 * \param[in]  key       Pointer to input key.
 * \param[in]  key_size  Size of input key.
 *
 * \return A status indicating the success/failure of the operation as specified
 *         in \ref psa_status_t
 *
 */
psa_status_t psa_adac_context_load_key(validation_context_t *context, uint8_t key_type,
                                       uint8_t *key, size_t key_size);

/**
 * \brief Performs certificate sanity checks to insure memory safety
 *
 * \param[in] crt       Pointer to input certificate.
 * \param[in] crt_size  Size of input certificate.
 *
 * \return A status indicating the success/failure of the operation as specified
 *         in \ref psa_status_t
 *
 */
psa_status_t psa_adac_certificate_sanity_check(uint8_t *crt, size_t crt_size);

/**
 * \brief Extracts public key from input certificate
 *
 * \param[in]  crt          Pointer to input certificate.
 * \param[in]  crt_size     Size of input certificate.
 * \param[in]  key_type     Pointer to key type.
 * \param[out] pubkey       Double Pointer to output public key.
 * \param[out] pubkey_size  Pointer to size of public key.
 *
 * \return A status indicating the success/failure of the operation as specified
 *         in \ref psa_status_t
 *
 */
psa_status_t psa_adac_extract_public_key(uint8_t *crt, size_t crt_size, uint8_t *key_type,
                                         uint8_t **pubkey, size_t *pubkey_size);

/**
 * \brief Update the key context from certificate.
 *
 * \param[in]  crt       Pointer to input certificate.
 * \param[in]  crt_size  Size of input certificate.
 * \param[out] context   Pointer to output context.
 *
 * \return A status indicating the success/failure of the operation as specified
 *         in \ref psa_status_t
 *
 */
psa_status_t psa_adac_update_context(uint8_t *crt, size_t crt_size, validation_context_t *context);

/**
 * \brief Verifies the certificate extensions and its signature
 *
 * \param[in] crt       Pointer to input certificate.
 * \param[in] crt_size  Size of input certificate.
 * \param[in] key_type  Type of input key.
 * \param[in] key       Pointer to input key.
 * \param[in] key_size  Size of input key.
 *
 * \return A status indicating the success/failure of the operation as specified
 *         in \ref psa_status_t
 *
 */
psa_status_t psa_adac_certificate_verify_sig(uint8_t *crt, size_t crt_size,
                                             uint8_t key_type, uint8_t *key, size_t key_size);

/**
 * \brief Verifies the token context
 *
 * \param[in]  token      Pointer to input token buffer.
 * \param[in]  token_size Size of token buffer.
 * \param[out] sig        Double pointer to extracted signature.
 * \param[out] sig_size   Pointer to size of extracted signature .
 * \param[out] tbs_size   Pointer to token before signature size.
 * \param[out] body_size  Pointer to token body size.
 * \param[out] hash_algo  Pointer to hash algorithm used.
 * \param[out] sig_algo   Pointer to algorithm used for signature.
 *
 * \return A status indicating the success/failure of the operation as specified
 *         in \ref psa_status_t
 *
 */
psa_status_t psa_adac_token_verify_info(uint8_t token[], size_t token_size, uint8_t **sig,
                                        size_t *sig_size, size_t *tbs_size, size_t *_body_size,
                                        psa_algorithm_t *hash_algo, psa_algorithm_t *sig_algo);
/**
 * \brief Verifies the token signature for authentication
 *
 * \param[in] token          Pointer to input token.
 * \param[in] token_size     Size of input token.
 * \param[in] challenge      Pointer to input challenge.
 * \param[in] challenge_size Size of input challenge.
 * \param[in] key_type       Type of input key.
 * \param[in] key            Pointer to input key.
 * \param[in] key_size       Size of input key.
 *
 * \return A status indicating the success/failure of the operation as specified
 *         in \ref psa_status_t
 *
 */
psa_status_t psa_adac_verify_token_signature(uint8_t *token, size_t token_size, uint8_t *challenge,
                                             size_t challenge_size, uint8_t key_type, uint8_t *key,
                                             size_t key_size);
/**
 * \brief Extracts & verifies root of trust public key from certificate
 *
 * \param[in] crt         Pointer to input certificate.
 * \param[in] crt_size    Size of input certificate.
 * \param[in] alg         Algorithm type used for signature.
 * \param[in] rotpk       Double Pointer to root of trust public key buffer.
 * \param[in] rotpk_size  Pointer to size of public key buffer.
 * \param[in] rotpk_count Number of keys to verify.
 *
 * \return A status indicating the success/failure of the operation as specified
 *         in \ref psa_status_t
 *
 */
psa_status_t psa_adac_verify_certificate_rotpk(uint8_t *crt, size_t crt_size, psa_algorithm_t alg,
                                               uint8_t **rotpk, size_t *rotpk_size,
                                               size_t rotpk_count);

/**@}*/

#ifdef __cplusplus
}
#endif

#endif /* __PSA_ADAC_H__ */
