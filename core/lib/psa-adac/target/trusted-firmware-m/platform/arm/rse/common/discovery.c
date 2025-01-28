/*
 * Copyright (c) 2022 Arm Limited. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include "psa_adac.h"
#include "psa_adac_config.h"

#include <stddef.h>
#include <stdint.h>

#ifdef PSA_ADAC_EC_P256
#define EC_P256_CNT 1
#define EC_P256_VAL ECDSA_P256_SHA256,
#else
#define EC_P256_CNT 0
#define EC_P256_VAL
#endif

#ifdef PSA_ADAC_EC_P521
#define EC_P521_CNT 1
#define EC_P521_VAL ECDSA_P521_SHA512,
#else
#define EC_P521_CNT 0
#define EC_P521_VAL
#endif

#ifdef PSA_ADAC_RSA3072
#define RSA3072_CNT 1
#define RSA3072_VAL RSA_3072_SHA256,
#else
#define RSA3072_CNT 0
#define RSA3072_VAL
#endif


#ifdef PSA_ADAC_RSA4096
#define RSA4096_CNT 1
#define RSA4096_VAL RSA_4096_SHA256,
#else
#define RSA4096_CNT 0
#define RSA4096_VAL
#endif

#ifdef PSA_ADAC_ED25519
#define ED25519_CNT 1
#define ED25519_VAL ED_25519_SHA512,
#else
#define ED25519_CNT 0
#define ED25519_VAL
#endif

#ifdef PSA_ADAC_ED448
#define ED448_CNT 1
#define ED448_VAL ED_448_SHAKE256,
#else
#define ED448_CNT 0
#define ED448_VAL
#endif

#ifdef PSA_ADAC_SM2SM3
#define SM2SM3_CNT 1
#define SM2SM3_VAL SM_SM2_SM3,
#else
#define SM2SM3_CNT 0
#define SM2SM3_VAL
#endif

#ifdef PSA_ADAC_HMAC
#define HMAC_CNT 1
#define HMAC_VAL CMAC_AES,
#else
#define HMAC_CNT 0
#define HMAC_VAL
#endif

#ifdef PSA_ADAC_CMAC
#define CMAC_CNT 1
#define CMAC_VAL HMAC_SHA256,
#else
#define CMAC_CNT 0
#define CMAC_VAL
#endif

#define CRYPTO_CNT EC_P256_CNT + EC_P521_CNT + RSA3072_CNT + RSA4096_CNT + \
    ED25519_CNT + ED448_CNT + SM2SM3_CNT + HMAC_CNT + CMAC_CNT
#define CRYPTO_VALS EC_P256_VAL EC_P521_VAL RSA3072_VAL RSA4096_VAL \
    ED25519_VAL ED448_VAL SM2SM3_VAL HMAC_VAL CMAC_VAL

uint8_t discovery_template[] = {
        /* @+00 (12 bytes) psa_auth_version: 1.0 */
        0x00, 0x00, 0x01, 0x00, 0x02, 0x00, 0x00, 0x00,
        0x01, 0x00, 0x00, 0x00,
        /* @+12 (12 bytes) vendor_id: {0x04, 0x3B} => 0x023B ("ARM Ltd.") */
        0x00, 0x00, 0x02, 0x00, 0x02, 0x00, 0x00, 0x00,
        0x04, 0x3B, 0x00, 0x00,
        /* @+24 (12 bytes) soc_class: [0x00, 0x00, 0x00, 0x00] */
        0x00, 0x00, 0x03, 0x00, 0x04, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00,
        /* @+36 (24 bytes) soc_id: [0x00] * 16 */
        0x00, 0x00, 0x04, 0x00, 0x10, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        /* @+60 (12 bytes) psa_lifecycle: PSA_LIFECYCLE_SECURED */
        0x00, 0x00, 0x08, 0x00, 0x02, 0x00, 0x00, 0x00,
        0x00, 0x30, 0x00, 0x00,
        /* @+72 (12 bytes) token_formats: [{0x00, 0x02} (token_psa_debug)] */
        0x00, 0x00, 0x00, 0x01, 0x02, 0x00, 0x00, 0x00,
        0x00, 0x02, 0x00, 0x00,
        /* @+84 (12 bytes) cert_formats: [{0x01, 0x02} (cert_psa_debug)] */
        0x00, 0x00, 0x01, 0x01, 0x02, 0x00, 0x00, 0x00,
        0x01, 0x02, 0x00, 0x00,
        /* @+96 (8 + X bytes) cryptosystems: [...] */
        0x00, 0x00, 0x02, 0x01, CRYPTO_CNT, 0x00, 0x00, 0x00, CRYPTO_VALS
        /* Maximum padding */
        0x00, 0x00, 0x00
};

size_t discovery_template_len = sizeof(discovery_template) - (sizeof(discovery_template) % 4);
