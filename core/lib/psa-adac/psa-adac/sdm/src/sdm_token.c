/*
 * Copyright (c) 2022, Arm Limited. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 */

#include "psa_adac_config.h"
#include "psa_adac.h"
#include "psa_adac_crypto_api.h"
#include "psa_adac_cryptosystems.h"
#include "psa_adac_debug.h"

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

static psa_status_t psa_adac_pki_sign(psa_algorithm_t sig_algo, psa_algorithm_t hash_algo,
                               const uint8_t *inputs[], size_t input_sizes[], size_t input_count,
                               psa_key_handle_t handle, uint8_t sig[], size_t sig_size,
                               size_t *sig_length)
{
    uint8_t hash[PSA_HASH_MAX_SIZE];
    size_t hash_size;
    psa_status_t status = psa_adac_hash_multiple(hash_algo, inputs, input_sizes, input_count,
                                            hash, sizeof(hash), &hash_size);
    if (PSA_SUCCESS != status) {
        PSA_ADAC_LOG_ERR("token", "Error hashing (%d)\n", status);
    } else if (PSA_SUCCESS != (status = psa_sign_hash(handle, sig_algo, hash, hash_size, sig,
                                                      sig_size, sig_length))) {
        PSA_ADAC_LOG_ERR("token", "Error signing (%d)\n", status);
    }

    return status;
}

static psa_status_t psa_adac_mac_sign(psa_algorithm_t algo, const uint8_t *inputs[], size_t input_sizes[],
                               size_t input_count, const uint8_t key[], size_t key_size,
                               uint8_t mac[], size_t mac_length, size_t *mac_size)
{
    psa_status_t status;
    psa_key_attributes_t attributes = PSA_KEY_ATTRIBUTES_INIT;
    psa_key_handle_t handle;
    psa_key_type_t kt;
    size_t length, bits;

    if (algo == PSA_ALG_CMAC) {
        if (key_size != 16) {
            return PSA_ERROR_INVALID_ARGUMENT;
        }
        kt = PSA_KEY_TYPE_AES;
        bits = 128;
    } else if (algo == PSA_ALG_HMAC(PSA_ALG_SHA_256)) {
        if (key_size != 32) {
            return PSA_ERROR_INVALID_ARGUMENT;
        }
        kt = PSA_KEY_TYPE_HMAC;
        bits = 256;
    } else {
        return PSA_ERROR_NOT_SUPPORTED;
    }

    psa_set_key_usage_flags(&attributes, 0);
    psa_set_key_algorithm(&attributes, algo);
    psa_set_key_usage_flags(&attributes, PSA_KEY_USAGE_SIGN_HASH);
    psa_set_key_type(&attributes, kt);
    psa_set_key_bits(&attributes, bits);

    psa_mac_operation_t operation = psa_mac_operation_init();

    if (PSA_SUCCESS == (status = psa_import_key(&attributes, key, key_size, &handle))) {
        if (PSA_SUCCESS == (status = psa_mac_sign_setup(&operation, handle, algo))) {
            for (int i = 0; (i < input_count) && (status == PSA_SUCCESS); i++) {
                status = psa_mac_update(&operation, inputs[i], input_sizes[i]);
            }

            if (PSA_SUCCESS == status) {
                status = psa_mac_sign_finish(&operation, mac, mac_length, mac_size);
            }
        }

        psa_destroy_key(handle);
    }

    return status;
}

psa_status_t psa_adac_sign_token(uint8_t challenge[], size_t challenge_size, uint8_t signature_type,
                                 uint8_t exts[], size_t exts_size, uint8_t *fragment[],
                                 size_t *fragment_size, uint8_t req_perms[],
                                 psa_key_handle_t handle, uint8_t *key, size_t key_size)
{
    uint8_t hash[PSA_HASH_MAX_SIZE], *sig, *ext_hash, *_fragment;
    size_t token_size, hash_size, sig_size, body_size, tbs_size, ext_hash_size;
    psa_algorithm_t hash_algo, sig_algo;
    psa_status_t status;
    size_t ign = 0;

    if (signature_type == ECDSA_P256_SHA256) {
#ifdef PSA_ADAC_EC_P256
        token_size = sizeof(token_p256_t) + exts_size;
        _fragment = (uint8_t *) calloc(1, token_size + sizeof(psa_tlv_t));
        token_p256_t *token = (token_p256_t *) (_fragment + sizeof(psa_tlv_t));
        tbs_size = token->signature - (uint8_t *) token;
        body_size = sizeof(*token);
        sig = token->signature;
        sig_size = sizeof(token->signature);
        hash_algo = ECDSA_P256_HASH_ALGORITHM;
        sig_algo = ECDSA_P256_SIGN_ALGORITHM;
        ext_hash = token->extensions_hash;
        ext_hash_size = sizeof(token->extensions_hash);
#else
        return PSA_ERROR_NOT_SUPPORTED;
#endif /* PSA_ADAC_EC_P256 */
    } else if (signature_type == ECDSA_P521_SHA512) {
#ifdef PSA_ADAC_EC_P521
        token_size = sizeof(token_p521_t) + exts_size;
        _fragment = (uint8_t *) calloc(1, token_size + sizeof(psa_tlv_t));
        token_p521_t *token = (token_p521_t *) (_fragment + sizeof(psa_tlv_t));
        tbs_size = token->signature - (uint8_t *) token;
        body_size = sizeof(*token);
        sig = token->signature;
        sig_size = sizeof(token->signature);
        hash_algo = ECDSA_P521_HASH_ALGORITHM;
        sig_algo = ECDSA_P521_SIGN_ALGORITHM;
        ext_hash = token->extensions_hash;
        ext_hash_size = sizeof(token->extensions_hash);
#else
        return PSA_ERROR_NOT_SUPPORTED;
#endif /* PSA_ADAC_EC_P521 */
    } else if (signature_type == RSA_3072_SHA256) {
#ifdef PSA_ADAC_RSA3072
        body_size = sizeof(token_rsa3072_t);
        token_size = sizeof(token_rsa3072_t) + exts_size;
        _fragment = (uint8_t *) calloc(1, token_size + sizeof(psa_tlv_t));
        token_rsa3072_t *token = (token_rsa3072_t *) (_fragment + sizeof(psa_tlv_t));
        tbs_size = token->signature - (uint8_t *) token;
        sig = token->signature;
        sig_size = sizeof(token->signature);
        hash_algo = RSA_3072_HASH_ALGORITHM;
        sig_algo = RSA_3072_SIGN_ALGORITHM;
        ext_hash = token->extensions_hash;
        ext_hash_size = sizeof(token->extensions_hash);
#else
        return PSA_ERROR_NOT_SUPPORTED;
#endif /* PSA_ADAC_RSA3072 */
    } else if (signature_type == RSA_4096_SHA256) {
#ifdef PSA_ADAC_RSA4096
        token_size = sizeof(token_rsa4096_t) + exts_size;
        _fragment = (uint8_t *) calloc(1, token_size + sizeof(psa_tlv_t));
        token_rsa4096_t *token = (token_rsa4096_t *) (_fragment + sizeof(psa_tlv_t));
        tbs_size = token->signature - (uint8_t *) token;
        body_size = sizeof(*token);
        sig = token->signature;
        sig_size = sizeof(token->signature);
        hash_algo = RSA_4096_HASH_ALGORITHM;
        sig_algo = RSA_4096_SIGN_ALGORITHM;
        ext_hash = token->extensions_hash;
        ext_hash_size = sizeof(token->extensions_hash);
#else
        return PSA_ERROR_NOT_SUPPORTED;
#endif /* PSA_ADAC_RSA4096 */
    } else if (signature_type == ED_25519_SHA512) {
#ifdef PSA_ADAC_ED25519
        token_size = sizeof(token_ed255_t) + exts_size;
        _fragment = (uint8_t *) calloc(1, token_size + sizeof(psa_tlv_t));
        token_ed255_t *token = (token_ed255_t *) (_fragment + sizeof(psa_tlv_t));
        tbs_size = token->signature - (uint8_t *) token;
        body_size = sizeof(*token);
        sig = token->signature;
        sig_size = sizeof(token->signature);
        hash_algo = EDDSA_ED25519_HASH_ALGORITHM;
        sig_algo = EDDSA_ED25519_SIGN_ALGORITHM;
        ext_hash = token->extensions_hash;
        ext_hash_size = sizeof(token->extensions_hash);
#else
        return PSA_ERROR_NOT_SUPPORTED;
#endif /* PSA_ADAC_ED25519 */
    } else if (signature_type == ED_448_SHAKE256) {
#ifdef PSA_ADAC_ED448
        token_size = sizeof(token_ed448_t) + exts_size;
        _fragment = (uint8_t *) calloc(1, token_size + sizeof(psa_tlv_t));
        token_ed448_t *token = (token_ed448_t *) (_fragment + sizeof(psa_tlv_t));
        tbs_size = token->signature - (uint8_t *) token;
        body_size = sizeof(*token);
        sig = token->signature;
        sig_size = sizeof(token->signature);
        hash_algo = EDDSA_ED448_HASH_ALGORITHM;
        sig_algo = EDDSA_ED448_SIGN_ALGORITHM;
        ext_hash = token->extensions_hash;
        ext_hash_size = sizeof(token->extensions_hash);
#else
        return PSA_ERROR_NOT_SUPPORTED;
#endif /* PSA_ADAC_ED448 */
    } else if (signature_type == SM_SM2_SM3) {
#ifdef PSA_ADAC_SM2
        token_size = sizeof(token_sm2sm3_t) + exts_size;
        _fragment = (uint8_t *) calloc(1, token_size + sizeof(psa_tlv_t));
        token_sm2sm3_t *token = (token_sm2sm3_t *) (_fragment + sizeof(psa_tlv_t));
        tbs_size = token->signature - (uint8_t *) token;
        body_size = sizeof(*token);
        sig = token->signature;
        sig_size = sizeof(token->signature);
        hash_algo = SM2_SM3_HASH_ALGORITHM;
        sig_algo = SM2_SM3_SIGN_ALGORITHM;
        ext_hash = token->extensions_hash;
        ext_hash_size = sizeof(token->extensions_hash);
#else
        return PSA_ERROR_NOT_SUPPORTED;
#endif /* PSA_ADAC_SM2 */
    } else if (signature_type == CMAC_AES) {
#ifdef PSA_ADAC_CMAC
        token_size = sizeof(token_cmac_t) + exts_size;
        _fragment = (uint8_t *) calloc(1, token_size + sizeof(psa_tlv_t));
        token_cmac_t *token = (token_cmac_t *) (_fragment + sizeof(psa_tlv_t));
        tbs_size = token->signature - (uint8_t *) token;
        body_size = sizeof(*token);
        sig = token->signature;
        sig_size = sizeof(token->signature);
        hash_algo = CMAC_HASH_ALGORITHM;
        sig_algo = CMAC_SIGN_ALGORITHM;
        ext_hash = token->extensions_hash;
        ext_hash_size = sizeof(token->extensions_hash);
#else
        return PSA_ERROR_NOT_SUPPORTED;
#endif /* PSA_ADAC_CMAC */
    } else if (signature_type == HMAC_SHA256) {
#ifdef PSA_ADAC_HMAC
        token_size = sizeof(token_hmac_t) + exts_size;
        _fragment = (uint8_t *) calloc(1, token_size + sizeof(psa_tlv_t));
        token_hmac_t *token = (token_hmac_t *) (_fragment + sizeof(psa_tlv_t));
        tbs_size = token->signature - (uint8_t *) token;
        body_size = sizeof(*token);
        sig = token->signature;
        sig_size = sizeof(token->signature);
        hash_algo = HMAC_HASH_ALGORITHM;
        sig_algo = HMAC_SIGN_ALGORITHM;
        ext_hash = token->extensions_hash;
        ext_hash_size = sizeof(token->extensions_hash);
#else
        return PSA_ERROR_NOT_SUPPORTED;
#endif /* PSA_ADAC_HMAC */
    } else {
        return PSA_ERROR_NOT_SUPPORTED;
    }

    ((psa_tlv_t *) _fragment)->_reserved = 0x0;
    ((psa_tlv_t *) _fragment)->type_id = PSA_BINARY_TOKEN;
    ((psa_tlv_t *) _fragment)->length_in_bytes = token_size;

    token_header_t *token = (token_header_t *) (_fragment + sizeof(psa_tlv_t));
    token->format_version.major = ADAC_TOKEN_MAJOR;
    token->format_version.minor = ADAC_TOKEN_MINOR;
    token->signature_type = signature_type;
    token->extensions_bytes = exts_size;

    if (req_perms != NULL) {
        memcpy((void*) (token->requested_permissions), req_perms, PERMISSION_BITS / 8);
    } else {
        memset((void*) (token->requested_permissions), 0xFF, PERMISSION_BITS / 8);
    }

    if (exts_size > 0) {
        /* FIXME: Support PSA_ALG_CMAC */
        psa_adac_hash(hash_algo, exts, exts_size, ext_hash, ext_hash_size, &hash_size);
        memcpy((void *) (token + body_size), exts, exts_size);
    } else {
        memset((void *) ext_hash, 0, ext_hash_size);
    }


    const uint8_t *inputs[2] = {_fragment + sizeof(psa_tlv_t), challenge};
    size_t input_sizes[2] = {tbs_size, challenge_size};

    if ((sig_algo == CMAC_SIGN_ALGORITHM) || (sig_algo == HMAC_SIGN_ALGORITHM)) {
        status = psa_adac_mac_sign(sig_algo, inputs, input_sizes, 2, key, key_size,
                                   sig, sig_size, &ign);
    } else {
        status = psa_adac_pki_sign(sig_algo, hash_algo, inputs, input_sizes, 2, handle,
                                   sig, sig_size, &ign);
    }

    if (PSA_SUCCESS != status) {
        PSA_ADAC_LOG_ERR("token", "Error signing (%d)\n", status);
    } else {
        *fragment = _fragment;
        *fragment_size = token_size + sizeof(psa_tlv_t);
    }

    return status;
}
