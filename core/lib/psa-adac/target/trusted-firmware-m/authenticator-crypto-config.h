/*
 * Copyright (c) 2020, Arm Limited. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 */


#ifndef AUTHENTICATOR_CRYPTO_CONFIG_H
#define AUTHENTICATOR_CRYPTO_CONFIG_H

#include <psa_adac_config.h>

#define MBEDTLS_PSA_CRYPTO_C

/* System support */
#define MBEDTLS_PLATFORM_C
#define MBEDTLS_PLATFORM_MEMORY
#define MBEDTLS_MEMORY_BUFFER_ALLOC_C
#define MBEDTLS_NO_PLATFORM_ENTROPY
#define MBEDTLS_ENTROPY_HARDWARE_ALT
#define MBEDTLS_HAVE_ASM

#define MBEDTLS_PLATFORM_EXIT_ALT
#define MBEDTLS_PLATFORM_PRINTF_ALT

#if defined(PSA_ADAC_RSA3072) || defined(PSA_ADAC_RSA4096)
#define MBEDTLS_RSA_C
#define MBEDTLS_PKCS1_V21
#define MBEDTLS_OID_C
#define MBEDTLS_BIGNUM_C
#define MBEDTLS_PK_PARSE_C
#define MBEDTLS_PK_C

/* Support RSA key sizes up to 4096 bit */
#define MBEDTLS_MPI_MAX_SIZE 512
#endif

/* PSA ADAC */
#if defined(PSA_ADAC_EC_P256) || defined(PSA_ADAC_EC_P521)
#define MBEDTLS_ECDSA_C
#define MBEDTLS_ECP_C
#define MBEDTLS_ASN1_PARSE_C
#define MBEDTLS_ASN1_WRITE_C
#ifndef MBEDTLS_BIGNUM_C
#define MBEDTLS_BIGNUM_C
#endif
#ifndef MBEDTLS_PK_C
#define MBEDTLS_PK_C
#endif
#if defined(PSA_ADAC_EC_P256)
#define MBEDTLS_ECP_DP_SECP256R1_ENABLED
#endif
#if defined(PSA_ADAC_EC_P521)
#define MBEDTLS_ECP_DP_SECP521R1_ENABLED
#endif
#endif

/* Needed by PSA Crypto API Implementation */
#define MBEDTLS_CTR_DRBG_C
#define MBEDTLS_ENTROPY_C
#define MBEDTLS_AES_C
#define MBEDTLS_AES_ROM_TABLES
#define MBEDTLS_AES_FEWER_TABLES

#define MBEDTLS_MD_C
#define MBEDTLS_SHA224_C
#define MBEDTLS_SHA256_C
#define MBEDTLS_SHA256_SMALLER
#if defined(PSA_ADAC_EC_P521) || defined(PSA_ADAC_ED25519)
#define MBEDTLS_SHA512_C
#define MBEDTLS_SHA512_SMALLER
#else
#define MBEDTLS_ENTROPY_FORCE_SHA256
#endif

#ifdef PSA_ADAC_USE_CRYPTOCELL
#define MBEDTLS_AES_ALT
#define MBEDTLS_SHA256_ALT
#define MBEDTLS_ENTROPY_HARDWARE_ALT
#else
#define MBEDTLS_CIPHER_C
#endif

#ifdef PSA_ADAC_CMAC
#define MBEDTLS_CMAC_C
#ifndef MBEDTLS_CIPHER_C
#define MBEDTLS_CIPHER_C
#endif
#endif

#ifdef PSA_ADAC_HMAC
#define MBEDTLS_HKDF_C
#endif

#include "mbedtls/check_config.h"

#endif /* AUTHENTICATOR_CRYPTO_CONFIG_H */
