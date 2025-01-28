/*
 * Copyright (c) 2020 Arm Limited. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause
 */

/**
 * \file
 *
 * Defines certificate and token structures for supported cryptosystems.
 */

#ifndef PSA_ADAC_CRYPTOSYSTEMS_H
#define PSA_ADAC_CRYPTOSYSTEMS_H

#include <psa/crypto.h>

#ifdef PSA_ADAC_EC_P256

/** \addtogroup ecdsap256
 * @{
 */

#define ECDSA_P256_PUBLIC_KEY_SIZE 64
#define ECDSA_P256_SIGNATURE_SIZE  64
#define ECDSA_P256_HASH_SIZE       32
#define ECDSA_P256_HASH_ALGORITHM  PSA_ALG_SHA_256
#define ECDSA_P256_SIGN_ALGORITHM  PSA_ALG_DETERMINISTIC_ECDSA(PSA_ALG_SHA_256)

/** \brief ADAC certificate structure for ECDSA with P-256 curve cryptosystem
 */
typedef struct {
    certificate_header_t header;
    uint8_t pubkey[ECDSA_P256_PUBLIC_KEY_SIZE]; // P-256 public key
    uint8_t extensions_hash[ECDSA_P256_HASH_SIZE]; // SHA-256 hash
    uint8_t signature[ECDSA_P256_SIGNATURE_SIZE]; // P-256 with SHA-256 signature
    uint32_t extensions[];
} certificate_p256_p256_t;

/** \brief ADAC token structure for ECDSA with P-256 curve cryptosystem
 */
typedef struct {
    token_header_t header;
    uint8_t extensions_hash[ECDSA_P256_HASH_SIZE]; // SHA-256 hash
    uint8_t signature[ECDSA_P256_SIGNATURE_SIZE]; // P-256 with SHA-256 signature
    uint32_t extensions[];
} token_p256_t;

/**@}*/

#endif // PSA_ADAC_EC_P256

#ifdef PSA_ADAC_EC_P521

/** \addtogroup ecdsap521
 * @{
 */

#define ECDSA_P521_PUBLIC_KEY_SIZE 132
#define ECDSA_P521_SIGNATURE_SIZE  132
#define ECDSA_P521_HASH_SIZE       64
#define ECDSA_P521_HASH_ALGORITHM  PSA_ALG_SHA_512
#define ECDSA_P521_SIGN_ALGORITHM  PSA_ALG_DETERMINISTIC_ECDSA(PSA_ALG_SHA_512)

/** \brief ADAC certificate structure for ECDSA with P-521 curve cryptosystem
 */
typedef struct {
    certificate_header_t header;
    uint8_t pubkey[ROUND_TO_WORD(ECDSA_P521_PUBLIC_KEY_SIZE)]; // P-521 public key
    uint8_t extensions_hash[ECDSA_P521_HASH_SIZE]; // SHA-512 hash
    uint8_t signature[ROUND_TO_WORD(ECDSA_P521_SIGNATURE_SIZE)]; // P-521 with SHA-512 signature
    uint32_t extensions[];
} certificate_p521_p521_t;

/** \brief ADAC token structure for ECDSA with P-521 curve cryptosystem
 */
typedef struct {
    token_header_t header;
    uint8_t extensions_hash[ECDSA_P521_HASH_SIZE]; // SHA-512 hash
    uint8_t signature[ECDSA_P521_SIGNATURE_SIZE]; // P-521 with SHA-512 signature
    uint32_t extensions[];
} token_p521_t;

/**@}*/

#endif // PSA_ADAC_EC_P521

#ifdef PSA_ADAC_RSA3072

/** \addtogroup rsa3072
 * @{
 */

#define RSA_3072_PUBLIC_KEY_SIZE 384
#define RSA_3072_SIGNATURE_SIZE  384
#define RSA_3072_HASH_SIZE       32
#define RSA_3072_HASH_ALGORITHM  PSA_ALG_SHA_256
#define RSA_3072_SIGN_ALGORITHM  PSA_ALG_RSA_PSS(PSA_ALG_SHA_256)

/** \brief ADAC certificate structure for RSA 3072-bit key cryptosystem
 */
typedef struct {
    certificate_header_t header;
    uint8_t pubkey[RSA_3072_PUBLIC_KEY_SIZE]; // RSA 3072-bit public key
    uint8_t extensions_hash[RSA_3072_HASH_SIZE]; // SHA-256 hash
    uint8_t signature[RSA_3072_SIGNATURE_SIZE]; // RSA with SHA-256 signature
    uint32_t extensions[];
} certificate_rsa3072_rsa3072_t;

/** \brief ADAC token structure for RSA 3072-bit key cryptosystem
 */
typedef struct {
    token_header_t header;
    uint8_t extensions_hash[RSA_3072_HASH_SIZE]; // SHA-256 hash
    uint8_t signature[RSA_3072_SIGNATURE_SIZE]; // RSA with SHA-256 signature
    uint32_t extensions[];
} token_rsa3072_t;

/**@}*/

#endif // PSA_ADAC_RSA3072

#ifdef PSA_ADAC_RSA4096

/** \addtogroup rsa4096
 * @{
 */

#define RSA_4096_PUBLIC_KEY_SIZE 512
#define RSA_4096_SIGNATURE_SIZE  512
#define RSA_4096_HASH_SIZE       32
#define RSA_4096_HASH_ALGORITHM  PSA_ALG_SHA_256
#define RSA_4096_SIGN_ALGORITHM  PSA_ALG_RSA_PSS(PSA_ALG_SHA_256)

/** \brief ADAC certificate structure for RSA 4096-bit key cryptosystem
 */
typedef struct {
    certificate_header_t header;
    uint8_t pubkey[RSA_4096_PUBLIC_KEY_SIZE]; // RSA 4096-bit public key
    uint8_t extensions_hash[RSA_4096_HASH_SIZE]; // SHA-256 hash
    uint8_t signature[RSA_4096_SIGNATURE_SIZE]; // RSA with SHA-256 signature
    uint32_t extensions[];
} certificate_rsa4096_rsa4096_t;

/** \brief ADAC token structure for RSA 4096-bit key cryptosystem
 */
typedef struct {
    token_header_t header;
    uint8_t extensions_hash[RSA_4096_HASH_SIZE]; // SHA-256 hash
    uint8_t signature[RSA_4096_SIGNATURE_SIZE]; // RSA with SHA-256 signature
    uint32_t extensions[];
} token_rsa4096_t;

/**@}*/

#endif // PSA_ADAC_RSA4096

#ifdef PSA_ADAC_ED25519

/** \addtogroup ed25519
 * @{
 */

/** \brief ADAC certificate structure for EdDSA with Curve25519 curve cryptosystem
 */
#define PSA_ALG_ED25519               (PSA_ALG_VENDOR_FLAG & 0x1)
#define EDDSA_ED25519_PUBLIC_KEY_SIZE 32
#define EDDSA_ED25519_SIGNATURE_SIZE  64
#define EDDSA_ED25519_HASH_SIZE       64
#define EDDSA_ED25519_HASH_ALGORITHM  PSA_ALG_SHA_512
#define EDDSA_ED25519_SIGN_ALGORITHM  PSA_ALG_ED25519  // Non-standard

/** \brief ADAC certificate structure for EdDSA with Curve25519 curve cryptosystem
 */
typedef struct {
    certificate_header_t header;
    uint8_t pubkey[ROUND_TO_WORD(EDDSA_ED25519_PUBLIC_KEY_SIZE)];
    uint8_t extensions_hash[EDDSA_ED25519_HASH_SIZE];
    uint8_t signature[ROUND_TO_WORD(EDDSA_ED25519_SIGNATURE_SIZE)];
    uint32_t extensions[];
} certificate_ed255_ed255_t;


/** \brief ADAC token structure for EdDSA with Curve25519 curve cryptosystem
 */
typedef struct {
    token_header_t header;
    uint8_t extensions_hash[EDDSA_ED25519_HASH_SIZE]; // SHA-512 hash
    uint8_t signature[EDDSA_ED25519_SIGNATURE_SIZE]; // Ed25519 signature
    uint32_t extensions[];
} token_ed255_t;

/**@}*/

#endif // PSA_ADAC_ED25519

#ifdef PSA_ADAC_ED448

/** \addtogroup ed448
 * @{
 */

#define PSA_ALG_ED448               (PSA_ALG_VENDOR_FLAG & 0x2)
#define PSA_ALG_SHAKE256            (PSA_ALG_VENDOR_FLAG & 0x3)
#define EDDSA_ED448_PUBLIC_KEY_SIZE 57
#define EDDSA_ED448_SIGNATURE_SIZE  114
#define EDDSA_ED448_HASH_SIZE       64
#define EDDSA_ED448_HASH_ALGORITHM  PSA_ALG_SHAKE256 // Non-standard
#define EDDSA_ED448_SIGN_ALGORITHM  PSA_ALG_ED448 // Non-standard

/** \brief ADAC certificate structure for EdDSA with Curve448 curve cryptosystem
 */
typedef struct {
    certificate_header_t header;
    uint8_t pubkey[ROUND_TO_WORD(EDDSA_ED448_PUBLIC_KEY_SIZE)];
    uint8_t extensions_hash[EDDSA_ED448_HASH_SIZE];
    uint8_t signature[ROUND_TO_WORD(EDDSA_ED448_SIGNATURE_SIZE)];
    uint32_t extensions[];
} certificate_ed448_ed448_t;

/** \brief ADAC token structure for EdDSA with Curve448 curve cryptosystem
 */
typedef struct {
    token_header_t header;
    uint8_t extensions_hash[EDDSA_ED448_HASH_SIZE]; // SHAKE256 hash
    uint8_t signature[ROUND_TO_WORD(EDDSA_ED448_SIGNATURE_SIZE)]; // Ed448 signature
    uint32_t extensions[];
} token_ed448_t;

/**@}*/

#endif // PSA_ADAC_ED448

#ifdef PSA_ADAC_SM2

/** \addtogroup sm2sm3
 * @{
 */

#define PSA_ALG_SM2             (PSA_ALG_VENDOR_FLAG & 0x4)
#define PSA_ALG_SM3             (PSA_ALG_VENDOR_FLAG & 0x5)
#define SM2_SM3_PUBLIC_KEY_SIZE 64
#define SM2_SM3_SIGNATURE_SIZE  64
#define SM2_SM3_HASH_SIZE       32
#define SM2_SM3_HASH_ALGORITHM  PSA_ALG_SM3
#define SM2_SM3_SIGN_ALGORITHM  PSA_ALG_SM2

/** \brief ADAC certificate structure for SM2 cryptosystem
 */
typedef struct {
    certificate_header_t header;
    uint8_t pubkey[SM2_SM3_PUBLIC_KEY_SIZE]; // SM2 public key
    uint8_t extensions_hash[SM2_SM3_HASH_SIZE]; // SM3 hash
    uint8_t signature[SM2_SM3_SIGNATURE_SIZE]; // SM2 with SM3 signature
    uint32_t extensions[];
} certificate_sm2sm3_sm2sm3_t;

/** \brief ADAC token structure for SM2 cryptosystem
 */
typedef struct {
    token_header_t header;
    uint8_t extensions_hash[SM2_SM3_HASH_SIZE]; // SM3 hash
    uint8_t signature[SM2_SM3_SIGNATURE_SIZE]; // SM2 with SM3 signature
    uint32_t extensions[];
} token_sm2sm3_t;

/**@}*/

#endif // PSA_ADAC_SM2

#ifdef PSA_ADAC_CMAC

/** \addtogroup cmac
 * @{
 */

#define CMAC_PUBLIC_KEY_SIZE 16
#define CMAC_SIGNATURE_SIZE  16
#define CMAC_HASH_SIZE       16
#define CMAC_HASH_ALGORITHM  PSA_ALG_CMAC
#define CMAC_SIGN_ALGORITHM  PSA_ALG_CMAC

/** \brief ADAC certificate structure for CMAC with AES-128 cryptosystem
 */
typedef struct {
    certificate_header_t header;
    uint8_t pubkey[CMAC_PUBLIC_KEY_SIZE]; // Nonce
    uint8_t extensions_hash[CMAC_HASH_SIZE]; // CMAC
    uint8_t signature[CMAC_SIGNATURE_SIZE]; // CMAC
    uint32_t extensions[];
} certificate_cmac_cmac_t;

/** \brief ADAC token structure for CMAC with AES-128 cryptosystem
 */
typedef struct {
    token_header_t header;
    uint8_t extensions_hash[CMAC_HASH_SIZE]; // CMAC
    uint8_t signature[CMAC_SIGNATURE_SIZE]; // CMAC
    uint32_t extensions[];
} token_cmac_t;

/**@}*/

#endif // PSA_ADAC_CMAC

#ifdef PSA_ADAC_HMAC

/** \addtogroup hmac
 * @{
 */

#define HMAC_PUBLIC_KEY_SIZE 32
#define HMAC_SIGNATURE_SIZE  32
#define HMAC_HASH_SIZE       32
#define HMAC_HASH_ALGORITHM  PSA_ALG_SHA_256
#define HMAC_SIGN_ALGORITHM  PSA_ALG_HMAC(PSA_ALG_SHA_256)

/** \brief ADAC certificate structure for HMAC with SHA-256 cryptosystem
 */
typedef struct {
    certificate_header_t header;
    uint8_t pubkey[HMAC_PUBLIC_KEY_SIZE]; // Nonce
    uint8_t extensions_hash[HMAC_HASH_SIZE]; // SHA-256 hash
    uint8_t signature[HMAC_SIGNATURE_SIZE]; // HMAC
    uint32_t extensions[];
} certificate_hmac_hmac_t;

/** \brief ADAC token structure for HMAC with SHA-256 cryptosystem
 */
typedef struct {
    token_header_t header;
    uint8_t extensions_hash[HMAC_HASH_SIZE]; // SHA-256 Hash
    uint8_t signature[HMAC_SIGNATURE_SIZE]; // HMAC-SHA-256
    uint32_t extensions[];
} token_hmac_t;

/**@}*/

#endif // PSA_ADAC_HMAC

#endif //PSA_ADAC_CRYPTOSYSTEMS_H
