/*
 * Copyright (c) 2020-2023 Arm Limited. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <psa_adac_config.h>

#include <psa_adac.h>
#include <psa_adac_crypto_api.h>
#include <psa_adac_cryptosystems.h>
#include <psa_adac_debug.h>

psa_status_t psa_adac_certificate_sanity_check(uint8_t *crt, size_t crt_size) {
    certificate_header_t *h_crt = (certificate_header_t *) crt;
    psa_status_t r = PSA_SUCCESS;

    if ((crt == NULL) || (sizeof(certificate_header_t) > crt_size)) {
        r = PSA_ERROR_INVALID_ARGUMENT;
    } else {
        size_t body_size = 0;

        if ((h_crt->key_type == ECDSA_P256_SHA256) && (h_crt->signature_type == ECDSA_P256_SHA256)) {
#ifdef PSA_ADAC_EC_P256
            body_size = sizeof(certificate_p256_p256_t);
#endif
        } else if ((h_crt->key_type == ECDSA_P521_SHA512) && (h_crt->signature_type == ECDSA_P521_SHA512)) {
#ifdef PSA_ADAC_EC_P521
            body_size = sizeof(certificate_p521_p521_t);
#endif
        } else if ((h_crt->key_type == RSA_3072_SHA256) && (h_crt->signature_type == RSA_3072_SHA256)) {
#ifdef PSA_ADAC_RSA3072
            body_size = sizeof(certificate_rsa3072_rsa3072_t);
#endif
        } else if ((h_crt->key_type == RSA_4096_SHA256) && (h_crt->signature_type == RSA_4096_SHA256)) {
#ifdef PSA_ADAC_RSA4096
            body_size = sizeof(certificate_rsa4096_rsa4096_t);
#endif
        } else if ((h_crt->key_type == ED_25519_SHA512) && (h_crt->signature_type == ED_25519_SHA512)) {
#ifdef PSA_ADAC_ED25519
            body_size = sizeof(certificate_ed255_ed255_t);
#endif
        } else if ((h_crt->key_type == ED_448_SHAKE256) && (h_crt->signature_type == ED_448_SHAKE256)) {
#ifdef PSA_ADAC_ED448
            body_size = sizeof(certificate_ed448_ed448_t);
#endif
        } else if ((h_crt->key_type == SM_SM2_SM3) && (h_crt->signature_type == SM_SM2_SM3)) {
#ifdef PSA_ADAC_SM2
            body_size = sizeof(certificate_sm2sm3_sm2sm3_t);
#endif
        } else if ((h_crt->key_type == CMAC_AES) && (h_crt->signature_type == CMAC_AES)) {
#ifdef PSA_ADAC_CMAC
            body_size = sizeof(certificate_cmac_cmac_t);
#endif
        } else if ((h_crt->key_type == HMAC_SHA256) && (h_crt->signature_type == HMAC_SHA256)) {
#ifdef PSA_ADAC_HMAC
            body_size = sizeof(certificate_hmac_hmac_t);
#endif
        } else {
            body_size = 0; /* [misra-c2012-15.7] */
        }

        if (body_size == 0UL) {
            r = PSA_ERROR_NOT_SUPPORTED;
        } else {
            size_t exts_size = ROUND_TO_WORD(h_crt->extensions_bytes);
            if ((body_size + exts_size) != crt_size) {
                /* Inconsistent size */
                r = PSA_ERROR_INVALID_ARGUMENT;
            }
        }
    }

    return r;
}

psa_status_t psa_adac_certificate_verify_extensions(uint8_t *exts, size_t exts_size, psa_algorithm_t hash_algo,
                                                    uint8_t key_type, uint8_t *key, size_t key_size,
                                                    uint8_t *hash, size_t hash_size) {
    psa_status_t r = PSA_ERROR_NOT_SUPPORTED;

    if ((hash_algo == PSA_ALG_SHA_256) || (hash_algo == PSA_ALG_SHA_512)) {
        r = psa_adac_hash_verify(hash_algo, exts, exts_size, hash, hash_size);
    } else if ((hash_algo == PSA_ALG_CMAC) && (key_type == CMAC_AES)) {
#if defined(PSA_ADAC_CMAC)
        r = psa_adac_mac_verify(hash_algo, (const uint8_t **) &exts, &exts_size, 1, key, key_size, hash, hash_size);
#else
        key;
        key_size;
#endif
    } else {
        r = PSA_ERROR_NOT_SUPPORTED; /* [misra-c2012-15.7] */
    }

    return r;
}

psa_status_t psa_adac_certificate_verify_sig(uint8_t *crt, size_t crt_size,
                                             uint8_t key_type, uint8_t *key, size_t key_size) {
    certificate_header_t *h_crt = (certificate_header_t *) crt;
    uint8_t *sig = NULL;
    uint8_t *ext = NULL;
    uint8_t *ext_hash = NULL;
    psa_status_t r = PSA_SUCCESS;
    psa_algorithm_t sig_algo = 0;
    psa_algorithm_t hash_algo = 0;
    size_t hash_size = 0;
    size_t sig_size = 0;
    size_t tbs_size = 0;
    size_t body_size = 0;

    if ((crt == NULL) || (key == NULL) || (crt_size <= 0UL) || (key_size <= 0UL)) {
        PSA_ADAC_LOG_ERR("crt", "Invalid arguments\r\n");
        r = PSA_ERROR_INVALID_ARGUMENT;
    } else if ((h_crt->key_type != key_type) || (h_crt->signature_type != key_type)) {
        PSA_ADAC_LOG_ERR("crt", "Types mismatch\r\n");
        r = PSA_ERROR_NOT_SUPPORTED;
    } else if ((h_crt->key_type == ECDSA_P256_SHA256) && (h_crt->signature_type == ECDSA_P256_SHA256)) {
#ifdef PSA_ADAC_EC_P256
        certificate_p256_p256_t *s_crt = (certificate_p256_p256_t *) crt;
        sig = s_crt->signature;
        sig_algo = ECDSA_P256_SIGN_ALGORITHM;
        sig_size = sizeof(s_crt->signature);
        tbs_size = offsetof(certificate_p256_p256_t, signature);
        body_size = sizeof(certificate_p256_p256_t);
        ext = (uint8_t *) s_crt->extensions;
        ext_hash = s_crt->extensions_hash;
        hash_size = sizeof(s_crt->extensions_hash);
        hash_algo = ECDSA_P256_HASH_ALGORITHM;
#else
        r = PSA_ERROR_NOT_SUPPORTED;
#endif
    } else if ((h_crt->key_type == ECDSA_P521_SHA512) && (h_crt->signature_type == ECDSA_P521_SHA512)) {
#ifdef PSA_ADAC_EC_P521
        certificate_p521_p521_t *s_crt = (certificate_p521_p521_t *) crt;
        sig = s_crt->signature;
        sig_algo = ECDSA_P521_SIGN_ALGORITHM;
        sig_size = sizeof(s_crt->signature);
        tbs_size = offsetof(certificate_p521_p521_t, signature);
        body_size = sizeof(certificate_p521_p521_t);
        ext = (uint8_t *) s_crt->extensions;
        ext_hash = s_crt->extensions_hash;
        hash_size = sizeof(s_crt->extensions_hash);
        hash_algo = ECDSA_P521_HASH_ALGORITHM;
#else
        r = PSA_ERROR_NOT_SUPPORTED;
#endif
    } else if ((h_crt->key_type == RSA_3072_SHA256) && (h_crt->signature_type == RSA_3072_SHA256)) {
#ifdef PSA_ADAC_RSA3072
        certificate_rsa3072_rsa3072_t *s_crt = (certificate_rsa3072_rsa3072_t *) crt;
        sig = s_crt->signature;
        sig_algo = RSA_3072_SIGN_ALGORITHM;
        sig_size = sizeof(s_crt->signature);
        tbs_size = offsetof(certificate_rsa3072_rsa3072_t, signature);
        body_size = sizeof(certificate_rsa3072_rsa3072_t);
        ext = (uint8_t *) s_crt->extensions;
        ext_hash = s_crt->extensions_hash;
        hash_size = sizeof(s_crt->extensions_hash);
        hash_algo = RSA_3072_HASH_ALGORITHM;
#else
        r = PSA_ERROR_NOT_SUPPORTED;
#endif
    } else if ((h_crt->key_type == RSA_4096_SHA256) && (h_crt->signature_type == RSA_4096_SHA256)) {
#ifdef PSA_ADAC_RSA4096
        certificate_rsa4096_rsa4096_t *s_crt = (certificate_rsa4096_rsa4096_t *) crt;
        sig = s_crt->signature;
        sig_algo = RSA_4096_SIGN_ALGORITHM;
        sig_size = sizeof(s_crt->signature);
        tbs_size = offsetof(certificate_rsa4096_rsa4096_t, signature);
        body_size = sizeof(certificate_rsa4096_rsa4096_t);
        ext = (uint8_t *) s_crt->extensions;
        ext_hash = s_crt->extensions_hash;
        hash_size = sizeof(s_crt->extensions_hash);
        hash_algo = RSA_4096_HASH_ALGORITHM;
#else
        r = PSA_ERROR_NOT_SUPPORTED;
#endif
    } else if ((h_crt->key_type == ED_25519_SHA512) && (h_crt->signature_type == ED_25519_SHA512)) {
#ifdef PSA_ADAC_ED25519
        certificate_ed255_ed255_t *s_crt = (certificate_ed255_ed255_t *) crt;
        sig = s_crt->signature;
        sig_algo = EDDSA_ED25519_SIGN_ALGORITHM;
        sig_size = sizeof(s_crt->signature);
        tbs_size = offsetof(certificate_ed255_ed255_t, signature);
        body_size = sizeof(certificate_ed255_ed255_t);
        ext = (uint8_t *) s_crt->extensions;
        ext_hash = s_crt->extensions_hash;
        hash_size = sizeof(s_crt->extensions_hash);
        hash_algo = EDDSA_ED25519_HASH_ALGORITHM;
        /* TODO */
#endif
        r = PSA_ERROR_NOT_SUPPORTED;
    } else if ((h_crt->key_type == ED_448_SHAKE256) && (h_crt->signature_type == ED_448_SHAKE256)) {
#ifdef PSA_ADAC_ED448
        certificate_ed448_ed448_t *s_crt = (certificate_ed448_ed448_t *) crt;
        sig = s_crt->signature;
        sig_algo = EDDSA_ED448_SIGN_ALGORITHM;
        sig_size = sizeof(s_crt->signature);
        tbs_size = offsetof(certificate_ed448_ed448_t, signature);
        body_size = sizeof(certificate_ed448_ed448_t);
        ext = (uint8_t *) s_crt->extensions;
        ext_hash = s_crt->extensions_hash;
        hash_size = sizeof(s_crt->extensions_hash);
        hash_algo = EDDSA_ED448_HASH_ALGORITHM;
        /* TODO */
#endif
        r = PSA_ERROR_NOT_SUPPORTED;
    } else if ((h_crt->key_type == SM_SM2_SM3) && (h_crt->signature_type == SM_SM2_SM3)) {
#ifdef PSA_ADAC_SM2
        certificate_sm2sm3_sm2sm3_t *s_crt = (certificate_sm2sm3_sm2sm3_t *) crt;
        sig = s_crt->signature;
        sig_algo = SM2_SM3_SIGN_ALGORITHM;
        sig_size = sizeof(s_crt->signature);
        tbs_size = offsetof(certificate_sm2sm3_sm2sm3_t, signature);
        body_size = sizeof(certificate_sm2sm3_sm2sm3_t);
        ext = (uint8_t *) s_crt->extensions;
        ext_hash = s_crt->extensions_hash;
        hash_size = sizeof(s_crt->extensions_hash);
        hash_algo = SM2_SM3_HASH_ALGORITHM;
        /* TODO */
#endif
        r = PSA_ERROR_NOT_SUPPORTED;
    } else if ((h_crt->key_type == CMAC_AES) && (h_crt->signature_type == CMAC_AES)) {
#ifdef PSA_ADAC_CMAC
        certificate_cmac_cmac_t *s_crt = (certificate_cmac_cmac_t *) crt;
        sig = s_crt->signature;
        sig_algo = CMAC_SIGN_ALGORITHM;
        sig_size = sizeof(s_crt->signature);
        tbs_size = offsetof(certificate_cmac_cmac_t, signature);
        body_size = sizeof(certificate_cmac_cmac_t);
        ext = (uint8_t *) s_crt->extensions;
        ext_hash = s_crt->extensions_hash;
        hash_size = sizeof(s_crt->extensions_hash);
        hash_algo = CMAC_HASH_ALGORITHM;
#else
        r = PSA_ERROR_NOT_SUPPORTED;
#endif
    } else if ((h_crt->key_type == HMAC_SHA256) && (h_crt->signature_type == HMAC_SHA256)) {
#ifdef PSA_ADAC_HMAC
        certificate_hmac_hmac_t *s_crt = (certificate_hmac_hmac_t *) crt;
        sig = s_crt->signature;
        sig_algo = HMAC_SIGN_ALGORITHM;
        sig_size = sizeof(s_crt->signature);
        tbs_size = offsetof(certificate_hmac_hmac_t, signature);
        body_size = sizeof(certificate_hmac_hmac_t);
        ext = (uint8_t *) s_crt->extensions;
        ext_hash = s_crt->extensions_hash;
        hash_size = sizeof(s_crt->extensions_hash);
        hash_algo = HMAC_HASH_ALGORITHM;
#else
        r = PSA_ERROR_NOT_SUPPORTED;
#endif
    } else {
        PSA_ADAC_LOG_ERR("crt", "Unsupported certificate type\r\n");
        r = PSA_ERROR_NOT_SUPPORTED;
    }

    if (PSA_SUCCESS == r) {
        size_t exts_size = ROUND_TO_WORD(h_crt->extensions_bytes);
        if (exts_size > 0UL) {
            if ((body_size + exts_size) != crt_size) {
                /* Inconsistent size */
                r = PSA_ERROR_INVALID_ARGUMENT;
            } else {
                r = psa_adac_certificate_verify_extensions(ext, exts_size, hash_algo, key_type,
                                                           key, key_size, ext_hash, hash_size);
                if (r != PSA_SUCCESS) {
                    /* Hash does not match */
                }
            }
        } else {
            /* TODO: Check all zeros */
        }
    }

    if (PSA_SUCCESS == r) {
        PSA_ADAC_LOG_TRACE("crt", "Starting  signature verification (%zu)\r\n", sig_size);
        r = psa_adac_verify_signature(key_type, key, key_size, hash_algo, (const uint8_t **) &crt, &tbs_size, 1,
                                      sig_algo, sig, sig_size);
        PSA_ADAC_LOG_DEBUG("crt", "Signature verification: %s\r\n", (r == PSA_SUCCESS) ? "success" : "failure");
    }

    return r;
}
