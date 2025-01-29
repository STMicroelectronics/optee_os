/*
 * Copyright (c) 2020-2023 Arm Limited. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <string.h>
#include "psa_adac_config.h"
#include "psa_adac.h"
#include "psa_adac_crypto_api.h"
#include "psa_adac_cryptosystems.h"
#include "psa_adac_debug.h"
#ifdef PSA_ADAC_TARGET
#include "platform/platform.h"
#endif /* #ifdef PSA_ADAC_TARGET */


psa_status_t psa_adac_init()
{
    static uint8_t psa_adac_init_done = 0;
    psa_status_t ret = PSA_SUCCESS;

    if (psa_adac_init_done == 0UL) {
#ifdef PSA_ADAC_TARGET
        psa_adac_platform_init();
#endif /* #ifdef PSA_ADAC_TARGET */
        ret = psa_adac_crypto_init();
        psa_adac_init_done = 1;
    }

    return ret;
}

psa_status_t psa_adac_extract_public_key(uint8_t *crt, size_t crt_size, uint8_t *key_type,
                                         uint8_t **pubkey, size_t *pubkey_size)
{
    certificate_header_t *header = (certificate_header_t *) crt;
    size_t ext_bytes = 0;
    size_t body_size = 0;
    psa_status_t ret = PSA_SUCCESS;

    if ((crt_size < sizeof(certificate_header_t)) || (key_type == NULL) ||
        (pubkey == NULL) || (pubkey_size == NULL)) {
        ret = PSA_ERROR_INVALID_ARGUMENT;
    } else if ((header->key_type == ECDSA_P256_SHA256) &&
               (header->signature_type == ECDSA_P256_SHA256)) {
#ifdef PSA_ADAC_EC_P256
        certificate_p256_p256_t *certificate = (certificate_p256_p256_t *) crt;
        body_size = sizeof(certificate_p256_p256_t);
        ext_bytes = certificate->header.extensions_bytes;
        *pubkey = certificate->pubkey;
        *pubkey_size = sizeof(certificate->pubkey);
#else
        ret = PSA_ERROR_NOT_SUPPORTED;
#endif
    } else if ((header->key_type == ECDSA_P521_SHA512) &&
               (header->signature_type == ECDSA_P521_SHA512)) {
#ifdef PSA_ADAC_EC_P521
        certificate_p521_p521_t *certificate = (certificate_p521_p521_t *) crt;
        body_size = sizeof(certificate_p521_p521_t);
        ext_bytes = certificate->header.extensions_bytes;
        *pubkey = certificate->pubkey;
        *pubkey_size = sizeof(certificate->pubkey);
#else
        ret = PSA_ERROR_NOT_SUPPORTED;
#endif
    } else if ((header->key_type == ED_25519_SHA512) &&
               (header->signature_type == ED_25519_SHA512)) {
#ifdef PSA_ADAC_ED25519
        certificate_ed255_ed255_t *certificate = (certificate_ed255_ed255_t *) crt;
        body_size = sizeof(certificate_ed255_ed255_t);
        ext_bytes = certificate->header.extensions_bytes;
        *pubkey = certificate->pubkey;
        *pubkey_size = sizeof(certificate->pubkey);
#else
        ret = PSA_ERROR_NOT_SUPPORTED;
#endif
    } else if ((header->key_type == ED_448_SHAKE256) &&
               (header->signature_type == ED_448_SHAKE256)) {
#ifdef PSA_ADAC_ED448
        certificate_ed448_ed448_t *certificate = (certificate_ed448_ed448_t *) crt;
        body_size = sizeof(certificate_ed448_ed448_t);
        ext_bytes = certificate->header.extensions_bytes;
        *pubkey = certificate->pubkey;
        *pubkey_size = sizeof(certificate->pubkey);
#else
        ret = PSA_ERROR_NOT_SUPPORTED;
#endif
    } else if ((header->key_type == RSA_3072_SHA256) &&
               (header->signature_type == RSA_3072_SHA256)) {
#ifdef PSA_ADAC_RSA3072
        certificate_rsa3072_rsa3072_t *certificate = (certificate_rsa3072_rsa3072_t *) crt;
        body_size = sizeof(certificate_rsa3072_rsa3072_t);
        ext_bytes = certificate->header.extensions_bytes;
        *pubkey = certificate->pubkey;
        *pubkey_size = sizeof(certificate->pubkey);
#else
        ret = PSA_ERROR_NOT_SUPPORTED;
#endif
    } else if ((header->key_type == RSA_4096_SHA256) &&
               (header->signature_type == RSA_4096_SHA256)) {
#ifdef PSA_ADAC_RSA4096
        certificate_rsa4096_rsa4096_t *certificate = (certificate_rsa4096_rsa4096_t *) crt;
        body_size = sizeof(certificate_rsa4096_rsa4096_t);
        ext_bytes = certificate->header.extensions_bytes;
        *pubkey = certificate->pubkey;
        *pubkey_size = sizeof(certificate->pubkey);
#else
        ret = PSA_ERROR_NOT_SUPPORTED;
#endif
    } else {
        ret = PSA_ERROR_NOT_SUPPORTED;
    }

    if (PSA_SUCCESS == ret) {
        *key_type = header->key_type;

        if ((ROUND_TO_WORD(ext_bytes) + body_size) != crt_size) {
            PSA_ADAC_LOG_ERR("crypto", "Inconsistent certificate size\n");
            // Inconsistent certificate size
            ret = PSA_ERROR_INVALID_ARGUMENT;
        }
    }

    return ret;
}

psa_status_t psa_adac_verify_certificate_rotpk(uint8_t *crt, size_t crt_size, psa_algorithm_t alg,
                                               uint8_t **rotpk, size_t *rotpk_size,
                                               size_t rotpk_count)
{
    size_t pubkey_size;
    uint8_t key_type;
    uint8_t *pubkey = NULL;

    psa_status_t ret = psa_adac_extract_public_key(crt, crt_size, &key_type, &pubkey, &pubkey_size);
    if (ret == PSA_SUCCESS) {
        ret = psa_adac_hash_verify_multiple(alg, pubkey, pubkey_size, rotpk, rotpk_size,
                                            rotpk_count);
    }

    PSA_ADAC_LOG_TRACE("auth_rotpk", "ROTPK Certificate verification %s\n",
                       (ret == PSA_SUCCESS) ? "successful" : "failed");
    return ret;
}

psa_status_t psa_adac_context_load_key(validation_context_t *context, uint8_t key_type,
                                       uint8_t *key, size_t key_size)
{
    psa_status_t ret = PSA_ERROR_INSUFFICIENT_STORAGE;
    if (context->max >= key_size) {
        (void) memcpy(context->content, key, key_size);
        context->key_type = key_type;
        context->size = key_size;
        ret = PSA_SUCCESS;
    }
    return ret;
}

psa_status_t psa_adac_update_context(uint8_t *crt, size_t crt_size, validation_context_t *ctx)
{
    psa_status_t ret;

    if ((ctx->key_type == CMAC_AES) || (ctx->key_type == HMAC_SHA256)) {
#if defined(PSA_ADAC_CMAC) || defined(PSA_ADAC_HMAC)
        ret = psa_adac_derive_key(crt, crt_size, ctx->key_type, ctx->content, ctx->size);
#else
        ret = PSA_ERROR_NOT_SUPPORTED;
#endif
    } else {
        size_t key_size;
        uint8_t key_type;
        uint8_t *key = NULL;

        ret = psa_adac_extract_public_key(crt, crt_size, &key_type, &key, &key_size);
        if (PSA_SUCCESS == ret) {
            ret = psa_adac_context_load_key(ctx, key_type, key, key_size);
        }
    }

    return ret;
}
