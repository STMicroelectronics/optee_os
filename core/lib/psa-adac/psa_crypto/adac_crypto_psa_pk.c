/*
 * Copyright (c) 2020-2024, Arm Limited. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 */

#include "adac_crypto_psa.h"
#include "psa_adac_debug.h"
#include "psa_adac_config.h"

#include <string.h>

#ifdef ADAC_STATIC_PUB_KEYS
/* Since thin psa crypto layer in BL2 does not copy the key data, statically
 * allocate key buffer
 */
#define ADAC_STATIC static
#else
#define ADAC_STATIC
#endif /* ADAC_STATIC_PUB_KEYS */

#if defined(PSA_ADAC_RSA3072) || defined(PSA_ADAC_RSA4096)
#define ENCODED_EXPONENT_SIZE 5U
static const uint8_t encoded_exponent[ENCODED_EXPONENT_SIZE] =
                                                 {0x02, 0x03, 0x01, 0x00, 0x01};
#endif

#ifdef PSA_ADAC_RSA3072

#define RSA3072_HEADER_SIZE 8U
#define RSA3072_KEY_SIZE 384U /* 3072 bits */
#define RSA3072_ENCODED_PUB_KEY_SIZE (RSA3072_HEADER_SIZE + \
                                      RSA3072_KEY_SIZE + \
                                      ENCODED_EXPONENT_SIZE)

/* If MSB of the unsigned modulus is set, then an extra byte (0x00) needs to be
 * inserted before modulus
 */
#define RSA3072_ENCODED_PUB_KEY_MAX_SIZE (RSA3072_ENCODED_PUB_KEY_SIZE + 1U)

static const uint8_t rsa3072_header[RSA3072_HEADER_SIZE] = {
    0x30,       /* Start of sequence */
    0x82,       /* Length field indicator */
    0x01, 0x89, /* 0x189 (hex) or 393 (dec) bytes is length of data to follow */
    0x02,       /* Integer tag */
    0x82,       /* Length field indicator */
    0x01, 0x80  /* 0x180 (hex) or 384 (dec) bytes modulus size */
};

/* RFC3279 Section 2.3.1  - For rsa public keys, it expects key format as DER
 * encoding of the representation defined by Algorithms and IDs for
 * Internet X.509 PKI Certificate & CRL Profile
 */
static psa_status_t load_rsa_3072_public_key(uint8_t *key,
                                             size_t key_size,
                                             psa_key_handle_t *handle)
{
    psa_status_t ret;
    ADAC_STATIC uint8_t pub_key[RSA3072_ENCODED_PUB_KEY_MAX_SIZE];
    size_t offset = RSA3072_HEADER_SIZE;
    size_t pub_size = RSA3072_ENCODED_PUB_KEY_SIZE;

    if (key_size == RSA_3072_PUBLIC_KEY_SIZE) {
        /* Copy the header */
        (void) memcpy(pub_key, rsa3072_header, sizeof(rsa3072_header));

        /* If MSB is set, modulus need to be prefixed by 0x00 value */
        if ((key[0] & (uint8_t) 0x80U) != 0x00U) {
            /* Insert 0x00 after header */
            pub_key[offset] = 0x00;
            /* Increase the lengths by 1 */
            pub_key[3] = 0x8aU;
            pub_key[7] = 0x81U;
            offset += 1UL;
            pub_size += 1UL;
        }

        (void) memcpy(&(pub_key[offset]), key, RSA_3072_PUBLIC_KEY_SIZE);
        offset += RSA_3072_PUBLIC_KEY_SIZE;
        (void) memcpy(&(pub_key[offset]), encoded_exponent,
                      sizeof(encoded_exponent));

        psa_key_attributes_t attributes = PSA_KEY_ATTRIBUTES_INIT;
        psa_set_key_usage_flags(&attributes, PSA_KEY_USAGE_VERIFY_HASH);
        psa_set_key_type(&attributes, PSA_KEY_TYPE_RSA_PUBLIC_KEY);
        psa_set_key_algorithm(&attributes, PSA_ALG_RSA_PSS(PSA_ALG_ANY_HASH));
        psa_set_key_lifetime(&attributes, PSA_KEY_LIFETIME_VOLATILE);
        psa_set_key_bits(&attributes, 3072);

        ret = psa_import_key(&attributes, pub_key, pub_size, handle);
    } else {
        ret = PSA_ERROR_INVALID_ARGUMENT;
    }

    return ret;
}

#endif /* PSA_ADAC_RSA3072 */

#ifdef PSA_ADAC_RSA4096

#define RSA4096_HEADER_SIZE 8U
#define RSA4096_KEY_SIZE 512U /* 4096 bits */
#define RSA4096_ENCODED_PUB_KEY_SIZE (RSA4096_HEADER_SIZE + \
                                      RSA4096_KEY_SIZE + \
                                      ENCODED_EXPONENT_SIZE)

/* If MSB of the unsigned modulus is set, then an extra byte (0x00) needs to be
 * inserted before modulus
 */
#define RSA4096_ENCODED_PUB_KEY_MAX_SIZE (RSA4096_ENCODED_PUB_KEY_SIZE + 1U)

static const uint8_t rsa4096_header[RSA4096_HEADER_SIZE] = {
    0x30,       /* Start of sequence */
    0x82,       /* Length field indicator */
    0x02, 0x09, /* 0x209 (hex) or 521 (dec) bytes is length of data to follow */
    0x02,       /* Integer tag */
    0x82,       /* Length field indicator */
    0x02, 0x00  /* 0x200 (hex) or 512 (dec) bytes modulus size */
};

/* RFC3279 Section 2.3.1  - For rsa public keys, it expects key format as DER
 * encoding of the representation defined by Algorithms and IDs for
 * Internet X.509 PKI Certificate & CRL Profile
 */
static psa_status_t load_rsa_4096_public_key(uint8_t *key,
                                             size_t key_size,
                                             psa_key_handle_t *handle)
{
    psa_status_t ret;
    ADAC_STATIC uint8_t pub_key[RSA4096_ENCODED_PUB_KEY_MAX_SIZE];
    size_t offset = RSA4096_HEADER_SIZE;
    size_t pub_size = RSA4096_ENCODED_PUB_KEY_SIZE;

    if (RSA_4096_PUBLIC_KEY_SIZE == key_size) {

        /* Copy the header */
        (void) memcpy(pub_key, rsa4096_header, sizeof(rsa4096_header));

        /* If MSB is set, modulus need to be prefixed by 0x00 value */
        if ((key[0] & (uint8_t) 0x80) != 0x00U) {
            /* Insert 0x00 after header */
            pub_key[offset] = 0x00;
            /* Increase the lengths by 1 */
            pub_key[3] = 0x0a;
            pub_key[7] = 0x01;
            offset += 1UL;
            pub_size += 1UL;
        }

        (void) memcpy(&(pub_key[offset]), key, RSA_4096_PUBLIC_KEY_SIZE);
        offset += RSA_4096_PUBLIC_KEY_SIZE;
        (void) memcpy(&(pub_key[offset]), encoded_exponent,
                        sizeof(encoded_exponent));

        psa_key_attributes_t attributes = PSA_KEY_ATTRIBUTES_INIT;
        psa_set_key_usage_flags(&attributes, PSA_KEY_USAGE_VERIFY_HASH);
        psa_set_key_type(&attributes, PSA_KEY_TYPE_RSA_PUBLIC_KEY);
        psa_set_key_algorithm(&attributes, PSA_ALG_RSA_PSS(PSA_ALG_ANY_HASH));
        psa_set_key_lifetime(&attributes, PSA_KEY_LIFETIME_VOLATILE);
        psa_set_key_bits(&attributes, 4096);

        ret = psa_import_key(&attributes, pub_key, pub_size, handle);
    } else {
        ret = PSA_ERROR_INVALID_ARGUMENT;
    }

    return ret;
}

#endif /* PSA_ADAC_RSA4096 */

#ifdef PSA_ADAC_EC_P256

static psa_status_t load_ecdsa_p256_public_key(uint8_t *key,
                                               size_t key_size,
                                               psa_key_handle_t *handle)
{
    psa_status_t ret;
    ADAC_STATIC uint8_t pub_key[ECDSA_P256_PUBLIC_KEY_SIZE + 1] = {0x04};

    if (ECDSA_P256_PUBLIC_KEY_SIZE == key_size) {

        (void) memcpy(&(pub_key[1]), key, ECDSA_P256_PUBLIC_KEY_SIZE);
        psa_key_attributes_t attributes = PSA_KEY_ATTRIBUTES_INIT;
        psa_set_key_usage_flags(&attributes, PSA_KEY_USAGE_VERIFY_HASH);
        psa_set_key_type(&attributes, PSA_KEY_TYPE_ECC_PUBLIC_KEY(PSA_ECC_FAMILY_SECP_R1));
        psa_set_key_algorithm(&attributes, PSA_ALG_DETERMINISTIC_ECDSA(PSA_ALG_SHA_256));
        psa_set_key_lifetime(&attributes, PSA_KEY_LIFETIME_VOLATILE);
        psa_set_key_bits(&attributes, 256);

        ret = psa_import_key(&attributes, pub_key, sizeof(pub_key), handle);
    } else {

        ret = PSA_ERROR_INVALID_ARGUMENT;
    }

    return ret;
}

#endif /* PSA_ADAC_EC_P256 */

#ifdef PSA_ADAC_EC_P521

static psa_status_t load_ecdsa_p521_public_key(uint8_t *key,
                                               size_t key_size,
                                               psa_key_handle_t *handle)
{
    psa_status_t ret;
    ADAC_STATIC uint8_t pub_key[ECDSA_P521_PUBLIC_KEY_SIZE + 1] = {0x04};

    if (ECDSA_P521_PUBLIC_KEY_SIZE == key_size) {

        (void) memcpy(&(pub_key[1]), key, ECDSA_P521_PUBLIC_KEY_SIZE);
        psa_key_attributes_t attributes = PSA_KEY_ATTRIBUTES_INIT;
        psa_set_key_usage_flags(&attributes, PSA_KEY_USAGE_VERIFY_HASH);
        psa_set_key_type(&attributes, PSA_KEY_TYPE_ECC_PUBLIC_KEY(PSA_ECC_FAMILY_SECP_R1));
        psa_set_key_algorithm(&attributes, PSA_ALG_DETERMINISTIC_ECDSA(PSA_ALG_SHA_512));
        psa_set_key_lifetime(&attributes, PSA_KEY_LIFETIME_VOLATILE);
        psa_set_key_bits(&attributes, 521);

        ret = psa_import_key(&attributes, pub_key, sizeof(pub_key), handle);
    } else {
        ret = PSA_ERROR_INVALID_ARGUMENT;
    }

    return ret;
}

#endif /* PSA_ADAC_EC_P521 */

psa_status_t psa_adac_load_public_key(uint8_t key_type,
                                      uint8_t *key,
                                      size_t key_size,
                                      psa_key_handle_t *handle)
{
    psa_status_t ret = PSA_ERROR_NOT_SUPPORTED;

    if (key_type == ECDSA_P256_SHA256) {
#ifdef PSA_ADAC_EC_P256
        PSA_ADAC_LOG_TRACE("psa-crypto", "Load EcdsaP256 Public-key\n");
        ret = load_ecdsa_p256_public_key(key, key_size, handle);
#endif /* PSA_ADAC_EC_P256 */
    } else if (key_type == ECDSA_P521_SHA512) {
#ifdef PSA_ADAC_EC_P521
        PSA_ADAC_LOG_TRACE("psa-crypto", "Load EcdsaP521 Public-key\n");
        ret = load_ecdsa_p521_public_key(key, key_size, handle);
#endif /* PSA_ADAC_EC_P521 */
    } else if (key_type == RSA_3072_SHA256) {
#ifdef PSA_ADAC_RSA3072
        PSA_ADAC_LOG_TRACE("psa-crypto", "Load Rsa3072 Public-key\n");
        ret = load_rsa_3072_public_key(key, key_size, handle);
#endif /* PSA_ADAC_RSA3072 */
    } else if (key_type == RSA_4096_SHA256) {
#ifdef PSA_ADAC_RSA4096
        PSA_ADAC_LOG_TRACE("psa-crypto", "Load Rsa4096 Public-key\n");
        ret = load_rsa_4096_public_key(key, key_size, handle);
#endif /* PSA_ADAC_RSA4096 */
    } else {
        ret = PSA_ERROR_NOT_SUPPORTED;
    }

    return ret;
}

psa_status_t psa_adac_verify_signature(uint8_t key_type,
                                       uint8_t *key,
                                       size_t key_size,
                                       psa_algorithm_t hash_algo,
                                       const uint8_t *inputs[],
                                       size_t input_sizes[],
                                       size_t input_count,
                                       psa_algorithm_t sig_algo,
                                       uint8_t *sig, size_t sig_size)
{
    uint8_t hash[PSA_HASH_MAX_SIZE];
    size_t hash_size;
    psa_key_handle_t handle;
    psa_status_t ret;

    if ((PSA_ALG_IS_VENDOR_DEFINED(sig_algo) != 0) ||
        (sig_algo == PSA_ALG_HMAC(PSA_ALG_SHA_256)) || (sig_algo == PSA_ALG_CMAC)) {
        ret = psa_adac_verify_vendor(key_type, key, key_size, hash_algo,
                                     inputs, input_sizes, input_count,
                                     sig_algo, sig, sig_size);
    } else {
        ret = psa_adac_load_public_key(key_type, key, key_size, &handle);
        if (PSA_SUCCESS != ret) {
            PSA_ADAC_LOG_ERR("psa-crypto", "Error loading public key (%d)\n", ret);
        } else {
            ret = psa_adac_hash_multiple(hash_algo, inputs, input_sizes, input_count,
                                         hash, sizeof(hash), &hash_size);
            if (PSA_SUCCESS != ret) {
                PSA_ADAC_LOG_ERR("psa-crypto", "Error hashing content (%d)\n", ret);
            } else {
                PSA_ADAC_LOG_TRACE("psa-crypto", "Verify signature\n");
                ret = psa_verify_hash(handle, sig_algo, hash, hash_size, sig, sig_size);
                PSA_ADAC_LOG_DEBUG("psa-crypto", "Signature verification %s\n",
                                   (ret == PSA_SUCCESS) ? "successful" : "failed");
            }

            psa_destroy_key(handle);
        }
    }

    return ret;
}
