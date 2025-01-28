/*
 * Copyright (c) 2020-2023 Arm Limited. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <stdlib.h>
#include "psa_adac_config.h"

#include "psa_adac.h"
#include "psa_adac_crypto_api.h"
#include "psa_adac_cryptosystems.h"
#include "psa_adac_debug.h"


psa_status_t psa_adac_token_verify_info(uint8_t token[], size_t token_size, uint8_t **sig, size_t *sig_size,
                                        size_t *tbs_size, size_t *body_size, psa_algorithm_t *hash_algo,
                                        psa_algorithm_t *sig_algo) {
    token_header_t *header = (token_header_t *) token;
    size_t ext_hash_size = 0;
    size_t _body_size = 0;
    size_t _tbs_size = 0;
    size_t _sig_size = 0;
    psa_algorithm_t _hash_algo = 0;
    psa_algorithm_t _sig_algo = 0;
    uint8_t *exts = NULL;
    uint8_t *ext_hash = NULL;
    uint8_t *_sig = NULL;
    psa_status_t ret = PSA_SUCCESS;

    if (header->signature_type == ECDSA_P256_SHA256) {
#ifdef PSA_ADAC_EC_P256
        token_p256_t *_token = (token_p256_t *) token;
        _body_size = sizeof(token_p256_t);
        _tbs_size = offsetof(token_p256_t, signature);
        _sig_size = sizeof(_token->signature);
        _sig = _token->signature;
        _hash_algo = ECDSA_P256_HASH_ALGORITHM;
        _sig_algo = ECDSA_P256_SIGN_ALGORITHM;
        ext_hash_size = sizeof(_token->extensions_hash);
        ext_hash = _token->extensions_hash;
        exts = (uint8_t *) _token->extensions;
#else
        ret = PSA_ERROR_NOT_SUPPORTED;
#endif
    } else if (header->signature_type == ECDSA_P521_SHA512) {
#ifdef PSA_ADAC_EC_P521
        token_p521_t *_token = (token_p521_t *) token;
        _body_size = sizeof(token_p521_t);
        _tbs_size = offsetof(token_p521_t, signature);
        _sig_size = sizeof(_token->signature);
        _sig = _token->signature;
        _hash_algo = ECDSA_P521_HASH_ALGORITHM;
        _sig_algo = ECDSA_P521_SIGN_ALGORITHM;
        ext_hash_size = sizeof(_token->extensions_hash);
        ext_hash = _token->extensions_hash;
        exts = (uint8_t *) _token->extensions;
#else
        ret = PSA_ERROR_NOT_SUPPORTED;
#endif
    } else if (header->signature_type == RSA_3072_SHA256) {
#ifdef PSA_ADAC_RSA3072
        token_rsa3072_t *_token = (token_rsa3072_t *) token;
        _body_size = sizeof(token_rsa3072_t);
        _tbs_size = offsetof(token_rsa3072_t, signature);
        _sig_size = sizeof(_token->signature);
        _sig = _token->signature;
        _hash_algo = RSA_3072_HASH_ALGORITHM;
        _sig_algo = RSA_3072_SIGN_ALGORITHM;
        ext_hash_size = sizeof(_token->extensions_hash);
        ext_hash = _token->extensions_hash;
        exts = (uint8_t *) _token->extensions;
#else
        ret = PSA_ERROR_NOT_SUPPORTED;
#endif
    } else if (header->signature_type == RSA_4096_SHA256) {
#ifdef PSA_ADAC_RSA4096
        token_rsa4096_t *_token = (token_rsa4096_t *) token;
        _body_size = sizeof(token_rsa4096_t);
        _tbs_size = offsetof(token_rsa4096_t, signature);
        _sig_size = sizeof(_token->signature);
        _sig = _token->signature;
        _hash_algo = RSA_4096_HASH_ALGORITHM;
        _sig_algo = RSA_4096_SIGN_ALGORITHM;
        ext_hash_size = sizeof(_token->extensions_hash);
        ext_hash = _token->extensions_hash;
        exts = (uint8_t *) _token->extensions;
#else
        ret = PSA_ERROR_NOT_SUPPORTED;
#endif
    } else if (header->signature_type == ED_25519_SHA512) {
#ifdef PSA_ADAC_ED25519
        /* TODO */
        token_ed255_t *_token = (token_ed255_t *) token;
        _body_size = sizeof(token_ed255_t);
        _tbs_size = offsetof(token_ed255_t, signature);
        _sig_size = sizeof(_token->signature);
        _sig = _token->signature;
        _hash_algo = EDDSA_ED25519_HASH_ALGORITHM;
        ext_hash_size = sizeof(_token->extensions_hash);
        ext_hash = _token->extensions_hash;
        exts = (uint8_t *) _token->extensions;
#else
        ret = PSA_ERROR_NOT_SUPPORTED;
#endif
    } else if (header->signature_type == ED_448_SHAKE256) {
#ifdef PSA_ADAC_ED448
        /* TODO */
        token_ed448_t *_token = (token_ed448_t *) token;
        _body_size = sizeof(token_ed448_t);
        _tbs_size = offsetof(token_ed448_t, signature);
        _sig_size = sizeof(_token->signature);
        _sig = _token->signature;
        _hash_algo = EDDSA_ED448_HASH_ALGORITHM;
        ext_hash_size = sizeof(_token->extensions_hash);
        ext_hash = _token->extensions_hash;
        exts = (uint8_t *) _token->extensions;
#else
        ret = PSA_ERROR_NOT_SUPPORTED;
#endif
    } else if (header->signature_type == SM_SM2_SM3) {
#ifdef PSA_ADAC_SM2
        /* TODO */
        token_sm2sm3_t *_token = (token_sm2sm3_t *) token;
        _body_size = sizeof(token_sm2sm3_t);
        _tbs_size = offsetof(token_sm2sm3_t, signature);
        _sig_size = sizeof(_token->signature);
        _sig = _token->signature;
        _hash_algo = SM2_SM3_HASH_ALGORITHM;
        _sig_algo = SM2_SM3_SIGN_ALGORITHM;
        ext_hash_size = sizeof(_token->extensions_hash);
        ext_hash = _token->extensions_hash;
        exts = (uint8_t *) _token->extensions;
#else
        ret = PSA_ERROR_NOT_SUPPORTED;
#endif
    } else if (header->signature_type == CMAC_AES) {
#ifdef PSA_ADAC_CMAC
        token_cmac_t *_token = (token_cmac_t *) token;
        _body_size = sizeof(token_cmac_t);
        _tbs_size = offsetof(token_cmac_t, signature);
        _sig_size = sizeof(_token->signature);
        _sig = _token->signature;
        _hash_algo = CMAC_HASH_ALGORITHM;
        _sig_algo = CMAC_SIGN_ALGORITHM;
        ext_hash_size = sizeof(_token->extensions_hash);
        ext_hash = _token->extensions_hash;
        exts = (uint8_t *) _token->extensions;
#else
        ret = PSA_ERROR_NOT_SUPPORTED;
#endif
    } else if (header->signature_type == HMAC_SHA256) {
#ifdef PSA_ADAC_HMAC
        token_hmac_t *_token = (token_hmac_t *) token;
        _body_size = sizeof(token_hmac_t);
        _tbs_size = offsetof(token_hmac_t, signature);
        _sig_size = sizeof(_token->signature);
        _sig = _token->signature;
        _hash_algo = HMAC_HASH_ALGORITHM;
        _sig_algo = HMAC_SIGN_ALGORITHM;
        ext_hash_size = sizeof(_token->extensions_hash);
        ext_hash = _token->extensions_hash;
        exts = (uint8_t *) _token->extensions;
#else
        ret = PSA_ERROR_NOT_SUPPORTED;
#endif
    } else {
        ret = PSA_ERROR_NOT_SUPPORTED;
    }

    if (PSA_SUCCESS == ret) {
        size_t exts_size = ROUND_TO_WORD(header->extensions_bytes);
        if ((_body_size + exts_size) != token_size) {
            PSA_ADAC_LOG_ERR("token", "Size inconsistency %zu + %zu != %zu\r\n", _body_size, exts_size, token_size);
            ret = PSA_ERROR_INVALID_ARGUMENT;
        } else {
            if (exts_size > 0UL) {
                /* FIXME: PSA_ALG_CMAC */
                if (psa_adac_hash_verify(_hash_algo, exts, exts_size, ext_hash, ext_hash_size) != PSA_SUCCESS) {
                    PSA_ADAC_LOG_ERR("token", "Token extension hash does not match\r\n");
                    ret = PSA_ERROR_INVALID_SIGNATURE;
                } else {
                    /* FIXME: Check for 0s */
                }
            }
        }
    }

    if (PSA_SUCCESS == ret) {
        *body_size = _body_size;
        *hash_algo = _hash_algo;
        *sig_algo = _sig_algo;
        *sig_size = _sig_size;
        *tbs_size = _tbs_size;
        *sig = _sig;
    }

    return ret;
}

psa_status_t psa_adac_verify_token_signature(uint8_t *token, size_t token_size, uint8_t *challenge,
                                             size_t challenge_size, uint8_t key_type, uint8_t *key, size_t key_size) {
    uint8_t *sig;
    size_t sig_size;
    size_t body_size;
    size_t tbs_size;
    psa_algorithm_t hash_algo;
    psa_algorithm_t sig_algo;

    psa_status_t ret = psa_adac_token_verify_info(token, token_size, &sig, &sig_size, &tbs_size,
                                                  &body_size, &hash_algo, &sig_algo);

    if (ret != PSA_SUCCESS) {
        PSA_ADAC_LOG_ERR("token", "Unsupported token signature format\r\n");
    } else {
        const uint8_t *parts[2] = {(uint8_t *) token, challenge};
        size_t part_sizes[2] = {tbs_size, challenge_size};
        ret = psa_adac_verify_signature(key_type, key, key_size, hash_algo, parts, part_sizes, 2,
                                        sig_algo, sig, sig_size);
    }

    PSA_ADAC_LOG_DEBUG("token", "Signature verification (%d): %s\r\n", ret, (ret == PSA_SUCCESS) ? "success" : "failure");

    return ret;
}
