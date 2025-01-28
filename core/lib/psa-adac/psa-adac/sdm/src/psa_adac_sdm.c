/*
 * Copyright (c) 2022-2023, Arm Limited. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 */

#include "psa_adac_sdm.h"

#if defined(MBEDTLS_FS_IO)
#define mbedtls_free free

#include "psa_adac.h"
#include "psa_adac_crypto_api.h"
#include "psa_adac_cryptosystems.h"
#include "psa_adac_debug.h"

#include "mbedtls/pk.h"
#include "mbedtls/base64.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MAX_BUFFER_SIZE 4096 /* for RSA 4096 bit key */

static size_t word_padded(size_t input)
{
    return (input + 3) & ~((size_t)3);
}

int split_tlv_static(uint32_t *input, size_t input_size, psa_tlv_t **tlvs,
                     size_t max_tlvs, size_t *tlv_count)
{
    psa_tlv_t *current;
    size_t ext_size;
    size_t left = input_size;
    uint8_t *cur_input = (uint8_t *) input;
    size_t count = 0;

    while (left > 0 && count < max_tlvs) {
        current = (psa_tlv_t *) cur_input;
        if (sizeof(psa_tlv_t) > left) {
            return -1;
        }
        ext_size = word_padded(sizeof(psa_tlv_t) + current->length_in_bytes);
        if (ext_size > left) {
            PSA_ADAC_LOG_ERR("sdm", "Extension size inconsistency (ext_size = %ld, left = %ld)\n",
                              ext_size, left);
            return -1;
        }
        left -= ext_size;
        cur_input += ext_size;
        count += 1;
    }

    if (left != 0) {
        PSA_ADAC_LOG_ERR("sdm", "Extension size inconsistency (left = %ld)\n", left);
        return -1;
    }

    cur_input = (uint8_t *) input;
    for (size_t i = 0; i < count; i++) {
        current = (psa_tlv_t *) cur_input;
        tlvs[i] = current;
        cur_input += word_padded(sizeof(psa_tlv_t) + current->length_in_bytes);
    }
    *tlv_count = count;
    return 0;
}

int split_extensions(uint32_t *input, size_t input_size, psa_tlv_t **extensions[],
                     size_t *extension_count)
{
    psa_tlv_t *current;
    int count = 0;
    size_t left = input_size;
    uint8_t *cur_input = (uint8_t *) input;

    while (left > 0) {
        current = (psa_tlv_t *) cur_input;
        if (sizeof(psa_tlv_t) > left) {
            return -1;
        }
        size_t ext_size = word_padded(sizeof(psa_tlv_t) + current->length_in_bytes);

        if (ext_size > left) {
            PSA_ADAC_LOG_ERR("sdm", "Extension size inconsistency (ext_size = %ld, left = %ld)\n",
                             ext_size, left);
            return -1;
        }
        left -= ext_size;
        cur_input += ext_size;
        count += 1;
    }

    if (left != 0) {
        PSA_ADAC_LOG_ERR("sdm", "Extension size inconsistency (left = %ld)\n", left);
        return -1;
    }

    *extensions = (psa_tlv_t **) malloc(sizeof(psa_tlv_t *) * count);
    cur_input = (uint8_t *) input;
    for (int i = 0; i < count; i++) {
        psa_tlv_t *current = (psa_tlv_t *) cur_input;
        (*extensions)[i] = current;
        cur_input += word_padded(sizeof(psa_tlv_t) + current->length_in_bytes);
    }
    *extension_count = count;
    return 0;
}

int load_secret_key(const char *key_file, uint8_t type, uint8_t **key, size_t *key_size) {
    unsigned char *base64;
    size_t base64_size, bits;
    psa_key_type_t key_type = 0;

    int rc = mbedtls_pk_load_file(key_file, &base64, &base64_size);
    if (rc != 0) {
        PSA_ADAC_LOG_ERR("sdm", "Error calling mbedtls_pk_load_file(%s)\n", key_file);
        return rc;
    }

    *key = malloc(base64_size);
    if (*key != NULL) {
        *key_size = 0;
        rc = mbedtls_base64_decode(*key, base64_size, key_size, base64, base64_size);
        if (rc != 0) {
            PSA_ADAC_LOG_ERR("sdm", "Error calling mbedtls_base64_decode, error code %d\n", rc);
            free(*key);
        }
    } else {
        rc = -1;
    }

    mbedtls_free(base64);
    return rc;
}

int import_private_key(const char *key_file, uint8_t *type, psa_key_handle_t *handle)
{
    mbedtls_pk_context pk;
    psa_status_t crypto_ret;
    int rc;
    uint8_t buffer[MAX_BUFFER_SIZE];
    int key_type, bit_len, key_len;
    int p_size, offset = 0;
    psa_key_attributes_t attributes = PSA_KEY_ATTRIBUTES_INIT;

    mbedtls_pk_init(&pk);

    rc = mbedtls_pk_parse_keyfile(&pk, key_file, NULL);
    if (rc < 0) {
        PSA_ADAC_LOG_ERR("sdm", "Error loading key file '%s'\n", key_file);
        return rc;
    }

    key_type = mbedtls_pk_get_type(&pk);
    bit_len = mbedtls_pk_get_bitlen(&pk);
    key_len = mbedtls_pk_write_key_der(&pk, buffer, sizeof(buffer));
    if (key_len < 0) {
        PSA_ADAC_LOG_ERR("sdm", "Error serializing key\n");
        return rc;
    }

    if (key_type == MBEDTLS_PK_RSA) {
        psa_set_key_usage_flags(&attributes, PSA_KEY_USAGE_SIGN_HASH);
        psa_set_key_type(&attributes, PSA_KEY_TYPE_RSA_KEY_PAIR);
        psa_set_key_algorithm(&attributes, PSA_ALG_RSA_PSS(PSA_ALG_ANY_HASH));
        psa_set_key_bits(&attributes, bit_len);

        if (bit_len == 3072) {
            PSA_ADAC_LOG_DEBUG("sdm", "Importing RSA 3072-bit key\n");
            *type = RSA_3072_SHA256;
        } else if (bit_len == 4096) {
            PSA_ADAC_LOG_DEBUG("sdm", "Importing RSA 4096-bit key\n");
            *type = RSA_4096_SHA256;
        } else {
            PSA_ADAC_LOG_ERR("sdm", "Invalid key size (%d)\n", bit_len);
            return -1;
        }

        /* mbedtls_pk_write_key_der function write from the end of the data buffer,
         * hence key is end-aligned to the buffer
         */
        crypto_ret = psa_import_key(&attributes, buffer + sizeof(buffer) - key_len, key_len, handle);
    } else if ((key_type == MBEDTLS_PK_ECKEY) || (key_type == MBEDTLS_PK_ECDSA)) {
        psa_set_key_usage_flags(&attributes, PSA_KEY_USAGE_SIGN_HASH);
        psa_set_key_bits(&attributes, bit_len);

        if (bit_len == 256) {
            PSA_ADAC_LOG_DEBUG("sdm", "Importing EC P-256 key\n");
            psa_set_key_type(&attributes, PSA_KEY_TYPE_ECC_KEY_PAIR(PSA_ECC_FAMILY_SECP_R1));
            psa_set_key_algorithm(&attributes, PSA_ALG_DETERMINISTIC_ECDSA(PSA_ALG_SHA_256));
            p_size = 32;
            offset = 7;
            *type = ECDSA_P256_SHA256;
        } else if (bit_len == 521) {
            PSA_ADAC_LOG_DEBUG("sdm", "Importing EC P-521 key\n");
            psa_set_key_type(&attributes, PSA_KEY_TYPE_ECC_KEY_PAIR(PSA_ECC_FAMILY_SECP_R1));
            psa_set_key_algorithm(&attributes, PSA_ALG_DETERMINISTIC_ECDSA(PSA_ALG_SHA_512));
            psa_set_key_bits(&attributes, 521);
            p_size = 66;
            offset = 8;
            *type = ECDSA_P521_SHA512;
        } else {
            PSA_ADAC_LOG_ERR("sdm", "Invalid curve size (%d)\n", bit_len);
            return -1;
        }

        /* Use below while debug -
         * PSA_ADAC_LOG_DUMP("sdm", "key", buffer + sizeof(buffer) - key_len + offset, p_size);
         */
        /* mbedtls_pk_write_key_der function write from the end of the data buffer,
         * hence key is end-aligned to the buffer
         */
        crypto_ret = psa_import_key(&attributes, buffer + sizeof(buffer) - key_len + offset,
                                    p_size, handle);
    } else {
        PSA_ADAC_LOG_ERR("sdm", "Unsupported algorithm\n");
        return -1;
    }

    return crypto_ret;
}

int load_trust_chain(const char *chain_file, uint8_t **chain, size_t *chain_size) {
    unsigned char *base64;
    size_t base64_size;
    int rc;

    rc = mbedtls_pk_load_file(chain_file, &base64, &base64_size);
    if (rc != 0) {
        PSA_ADAC_LOG_ERR("sdm", "Error calling mbedtls_pk_load_file(%s)\n", chain_file);
        return rc;
    }

    *chain = malloc(base64_size);
    if (*chain != NULL) {
        *chain_size = 0;
        rc = mbedtls_base64_decode(*chain, base64_size, chain_size, base64, base64_size);
        if (rc != 0) {
            PSA_ADAC_LOG_ERR("sdm", "Error calling mbedtls_base64_decode, error code %d\n", rc);
            free(*chain);
        }
    } else {
        rc = -1;
    }

    mbedtls_free(base64);
    return rc;
}

int load_trust_rotpk(const char *chain_file, psa_algorithm_t alg,
                     uint8_t *rotpk, size_t buffer_size, size_t *rotpk_size, uint8_t *rotpk_type) {
    size_t chain_size, root_size = 0, pubkey_size = 0, exts_count = 0;
    uint8_t *root = NULL, *pubkey = NULL, key_type;
    uint32_t *chain;
    psa_tlv_t **exts;
    psa_status_t status;

    int rc = load_trust_chain(chain_file, (uint8_t **) &chain, &chain_size);
    if (rc != 0) {
        PSA_ADAC_LOG_ERR("sdm", "Error loading trust chain (%s)\n", chain_file);
        exit(-1);
    }

    if (split_extensions(chain, chain_size, &exts, &exts_count) < 0) {
        return -1;
    }

    for (size_t i = 0; i < exts_count; i++) {
        if (exts[i]->type_id == 0x0201) {
            root = exts[i]->value;
            root_size = exts[i]->length_in_bytes;
            break;
        } else {
            PSA_ADAC_LOG_ERR("sdm", "Ignoring unknown certificate format (0x%04x)\n",
                             exts[i]->type_id);
        }
    }

    status = psa_adac_extract_public_key(root, root_size, &key_type, &pubkey, &pubkey_size);

    if (status == PSA_SUCCESS) {
        status = psa_adac_hash(alg, pubkey, pubkey_size, rotpk, buffer_size, rotpk_size);
        if (rotpk_type != NULL) {
            *rotpk_type = key_type;
        }
    } else if ((key_type == CMAC_AES) || (key_type == HMAC_SHA256)) {
        /* FIXME: Work-around for symmetric keys on server */
        *rotpk_size = (key_type == HMAC_SHA256) ? 32 : 16;
        memset(rotpk, 0, *rotpk_size);
        *rotpk_type = key_type;
        status = PSA_SUCCESS;
    }

    free(exts);
    return status;
}

#endif /* #if defined(MBEDTLS_FS_IO) */
