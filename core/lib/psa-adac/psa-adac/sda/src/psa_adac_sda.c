/*
 * Copyright (c) 2020-2023 Arm Limited. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <string.h>
#include "psa_adac_sda.h"
#include "platform/msg_interface.h"
#include "psa_adac.h"
#include "psa_adac_crypto_api.h"
#include "psa_adac_cryptosystems.h"
#include "psa_adac_debug.h"


#ifdef PSA_ADAC_AUTHENTICATOR_IMPLICIT_TRANSPORT

static inline request_packet_t *authenticator_request_packet_receive(authentication_context_t *auth_ctx) {
    (void) auth_ctx; /* misra-c2012-2.7 */
    return request_packet_receive();
}

static inline int authenticator_request_packet_release(authentication_context_t *auth_ctx, request_packet_t *packet) {
    (void) auth_ctx; /* misra-c2012-2.7 */
    return request_packet_release(packet);
}

static inline int authenticator_response_packet_release(authentication_context_t *auth_ctx, response_packet_t *packet) {
    (void) auth_ctx; /* misra-c2012-2.7 */
    return response_packet_release(packet);
}

static inline response_packet_t *authenticator_response_packet_build(authentication_context_t *auth_ctx,
                                                                     uint16_t status, uint8_t *data, size_t data_size) {
    (void) auth_ctx; /* misra-c2012-2.7 */
    return response_packet_build(status, data, data_size);
}

static inline response_packet_t *authenticator_response_packet_lock(authentication_context_t *auth_ctx,
                                                                    size_t *max_data_size) {
    (void) auth_ctx; /* misra-c2012-2.7 */
    return response_packet_lock(max_data_size);
}

static inline int authenticator_response_packet_send(authentication_context_t *auth_ctx, response_packet_t *packet) {
    (void) auth_ctx; /* misra-c2012-2.7 */
    return response_packet_send(packet);
}

#endif

int authenticator_send_response(authentication_context_t *auth_ctx, response_packet_t *response) {
    int r = 1;
    if (response != NULL) {
        if (authenticator_response_packet_send(auth_ctx, response) == 0) {
            r = 0;
        }
        (void) authenticator_response_packet_release(auth_ctx, response);
    }
    return r;
}

response_packet_t *authentication_discovery(authentication_context_t *auth_ctx, request_packet_t *request) {
    (void) authenticator_request_packet_release(auth_ctx, request);
    PSA_ADAC_LOG_DEBUG("auth", "Discovery\r\n");

    size_t max = 0;
    response_packet_t *r = authenticator_response_packet_lock(auth_ctx, &max);
    if (r != NULL) {
        size_t size = psa_adac_platform_discovery((uint8_t *) r->data, max);
        while (((size % 4UL) != 0UL) && (size < max)) {
            ((uint8_t *) r->data)[size] = 0x0;
            size += 1UL;
        }
        r->data_count = size >> 2;
        r->status = 0x0;
    }
    return r;
}

response_packet_t *authentication_start(authentication_context_t *auth_ctx, request_packet_t *request) {
    (void) authenticator_request_packet_release(auth_ctx, request);
    PSA_ADAC_LOG_DEBUG("auth", "Starting authentication\r\n");
    auth_ctx->state = AUTH_CHALLENGE;

    PSA_ADAC_LOG_DEBUG("auth", "Generating challenge (%d)\r\n", CHALLENGE_SIZE);
    auth_ctx->challenge.format_version.major = 0x01;
    auth_ctx->challenge.format_version.minor = 0x00;
    auth_ctx->challenge._reserved = 0x00;
    psa_adac_generate_challenge(auth_ctx->challenge.challenge_vector, sizeof(auth_ctx->challenge.challenge_vector));
    response_packet_t *response = authenticator_response_packet_build(auth_ctx, 0x0, (uint8_t *) &auth_ctx->challenge,
                                                                      sizeof(auth_ctx->challenge));
    return response;
}

int is_hashed_rotpk_entry(uint8_t key_type, psa_algorithm_t algo, size_t rotpk_size) {
    int ret = 1;
    if ((key_type == CMAC_AES) || (key_type == HMAC_SHA256)) {
        ret = 0;
    }

    if (algo == PSA_ALG_SHA_256) {
        if (rotpk_size != 32UL) {
            ret = 0;
        }
    } else {
        if (rotpk_size != PSA_HASH_LENGTH(algo)) {
            ret = 0;
        }
    }

    return ret;
}

psa_status_t psa_adac_certificate_check(uint8_t *crt, size_t crt_size, rotpk_context_t *rotpk_ctx,
                                        validation_context_t *context, int is_root) {
    certificate_header_t *header = (certificate_header_t *) crt;
    psa_status_t r = psa_adac_certificate_sanity_check(crt, crt_size);

    if (PSA_SUCCESS != r) {
        /* Certificate failed sanity check */
    } else if (is_root != 0) {
        int found = 0;
        for (int i = 0; (i < rotpk_ctx->rotpk_count) && (!found); i++) {
            if ((rotpk_ctx->rotpk_type[i] != header->key_type) ||
                (rotpk_ctx->rotpk_type[i] != header->signature_type)) {
                continue;
            }
            if (is_hashed_rotpk_entry(rotpk_ctx->rotpk_type[i], rotpk_ctx->rotpk_algo, rotpk_ctx->rotpk_size[i]) != 0) {
                size_t pubkey_size;
                uint8_t key_type;
                uint8_t *pubkey = NULL;

                if ((PSA_SUCCESS != psa_adac_extract_public_key(crt, crt_size, &key_type, &pubkey, &pubkey_size)) ||
                    (PSA_SUCCESS != psa_adac_hash_verify(rotpk_ctx->rotpk_algo, pubkey, pubkey_size,
                                                         rotpk_ctx->rotpk[i], rotpk_ctx->rotpk_size[i])) ||
                    (PSA_SUCCESS != psa_adac_context_load_key(context, key_type, pubkey, pubkey_size)) ||
                    (PSA_SUCCESS != psa_adac_certificate_verify_sig(crt, crt_size, context->key_type,
                                                                    context->content, context->size))) {
                    continue;
                }
                found = 1;
            } else {
                if ((PSA_SUCCESS != psa_adac_context_load_key(context, rotpk_ctx->rotpk_type[i], rotpk_ctx->rotpk[i],
                                                              rotpk_ctx->rotpk_size[i])) ||
                    (PSA_SUCCESS != psa_adac_certificate_verify_sig(crt, crt_size, context->key_type,
                                                                    context->content, context->size)) ||
                    (PSA_SUCCESS != psa_adac_update_context(crt, crt_size, context))) {
                    continue;
                }
                found = 1;
            }
        }

        r = found ? PSA_SUCCESS : PSA_ERROR_DOES_NOT_EXIST;
    } else {
        r = psa_adac_certificate_verify_sig(crt, crt_size, context->key_type,
                                            context->content, context->size);
        if (PSA_SUCCESS == r) {
            r = psa_adac_update_context(crt, crt_size, context);
        }
    }

    return r;
}

psa_status_t certificate_check(authentication_context_t *ctx, uint8_t *crt, size_t crt_size, int is_root) {
    psa_status_t r = psa_adac_certificate_check(crt, crt_size, &(ctx->rotpk_ctx), &(ctx->context), is_root);
    if (PSA_SUCCESS != r) {
        PSA_ADAC_LOG_ERR("auth", "Error validating %s\r\n", is_root ? "root certificate" : "certificate");
    } else {
        r = psa_adac_platform_check_certificate(crt, crt_size);
        if (PSA_SUCCESS != r) {
            PSA_ADAC_LOG_ERR("auth", "Certificate rejected by platform\r\n");
        }
    }

    return r;
}

psa_status_t authentication_crt(authentication_context_t *auth_ctx, uint8_t *crt, size_t crt_size) {
    psa_status_t r = ADAC_FAILURE;
    certificate_header_t *header = (certificate_header_t *) crt;

    /* Check format version before reading any other fields. */
    if (header->format_version.major != ADAC_CERT_MAJOR) {
        PSA_ADAC_LOG_ERR("auth", "Unsupported certificate major version\r\n");
        return r;
    }

    if (header->role == ADAC_CRT_ROLE_ROOT) {
        if ((auth_ctx->state != AUTH_CHALLENGE) && (auth_ctx->state != AUTH_ROT_META)) {
            PSA_ADAC_LOG_ERR("auth", "State inconsistent with receiving a root certificate\r\n");

        } else if (PSA_SUCCESS == certificate_check(auth_ctx, crt, crt_size, 1)) {
            auth_ctx->state = AUTH_ROOT;
            r = ADAC_NEED_MORE_DATA;

            for (size_t i = 0; i < sizeof(auth_ctx->permissions_mask); i++) {
                auth_ctx->permissions_mask[i] &= header->permissions_mask[i];
            }
        } else {
            r = ADAC_FAILURE; /* [misra-c2012-15.7] */
        }
    } else if ((header->role == ADAC_CRT_ROLE_INT) || (header->role == ADAC_CRT_ROLE_LEAF)) {
        if ((auth_ctx->state != AUTH_ROOT) && (auth_ctx->state != AUTH_CHAIN)) {
            PSA_ADAC_LOG_ERR("auth", "State inconsistent with receiving an intermediate or leaf certificate\r\n");
        } else if (PSA_SUCCESS == certificate_check(auth_ctx, crt, crt_size, 0)) {
            auth_ctx->state = (header->role == ADAC_CRT_ROLE_INT) ? AUTH_CHAIN : AUTH_LEAF;
            r = ADAC_NEED_MORE_DATA;

            for (size_t i = 0; i < sizeof(auth_ctx->permissions_mask); i++) {
                auth_ctx->permissions_mask[i] &= header->permissions_mask[i];
            }
        } else {
            r = ADAC_FAILURE; /* [misra-c2012-15.7] */
        }
    } else {
        PSA_ADAC_LOG_ERR("auth_crt", "Inconsistent certificate role\r\n");
    }

    if (r == ADAC_FAILURE) {
        PSA_ADAC_LOG_ERR("auth_crt", "Authentication failure\r\n");
        auth_ctx->state = AUTH_FAILURE;
    }
    return r;
}

psa_status_t authentication_token(authentication_context_t *auth_ctx, uint8_t *token, size_t token_size) {
    size_t sig_size;
    size_t body_size;
    size_t tbs_size;
    psa_algorithm_t hash_algo;
    psa_algorithm_t sig_algo;
    psa_status_t r = ADAC_FAILURE;
    uint8_t *sig;
    token_header_t *header = (token_header_t *) token;

    /* Check format version before reading any other fields. */
    if (header->format_version.major != ADAC_TOKEN_MAJOR) {
        PSA_ADAC_LOG_ERR("auth", "Unsupported token major version\r\n");
        return r;
    }

    if ((auth_ctx->state != AUTH_ROOT) && (auth_ctx->state != AUTH_LEAF)) {
        PSA_ADAC_LOG_ERR("auth", "State inconsistent with receiving a token\r\n");
    } else if (PSA_SUCCESS != psa_adac_token_verify_info(token, token_size, &sig, &sig_size, &tbs_size,
                                                         &body_size, &hash_algo, &sig_algo)) {
        PSA_ADAC_LOG_ERR("auth", "Error checking token\r\n");
    } else if (PSA_SUCCESS != psa_adac_platform_check_token(token, token_size)) {
        PSA_ADAC_LOG_ERR("auth", "Token rejected by platform\r\n");
    } else if (PSA_SUCCESS !=
               psa_adac_verify_token_signature(token, token_size, auth_ctx->challenge.challenge_vector,
                                               sizeof(auth_ctx->challenge.challenge_vector),
                                               auth_ctx->context.key_type, auth_ctx->context.content,
                                               auth_ctx->context.size)) {
        PSA_ADAC_LOG_ERR("auth", "Invalid token signature\r\n");
    } else {
        PSA_ADAC_LOG_INFO("auth", "Authentication successful\r\n");
        for (size_t i = 0; i < sizeof(auth_ctx->permissions_mask); i++) {
            auth_ctx->permissions_mask[i] &= header->requested_permissions[i];
        }

        /* TODO: report failures while applying permissions */
        psa_adac_apply_permissions(auth_ctx->permissions_mask);

        r = ADAC_SUCCESS;
        auth_ctx->state = AUTH_SUCCESS;
    }

    if (r == ADAC_FAILURE) {
        auth_ctx->state = AUTH_FAILURE;
    }
    return r;
}

response_packet_t *authentication_response(authentication_context_t *auth_ctx, request_packet_t *request) {
    PSA_ADAC_LOG_DEBUG("auth", "Received Authentication Response (%d)\r\n", request->data_count * 4);
    psa_tlv_t *fragment = (psa_tlv_t *) &request->data;
    psa_status_t r = ADAC_FAILURE;

    if (((request->data_count * 4) < sizeof(psa_tlv_t))
            || (((request->data_count * 4) - sizeof(psa_tlv_t)) < fragment->length_in_bytes)) {
        auth_ctx->state = AUTH_FAILURE;
        PSA_ADAC_LOG_DEBUG("auth", "Request size too small\r\n");
    } else if (fragment->type_id == PSA_BINARY_CRT) { /* TODO: Add support for RoT Metadata fragment */
        if ((auth_ctx->state != AUTH_CHALLENGE) && (auth_ctx->state != AUTH_ROT_META) &&
            (auth_ctx->state != AUTH_ROOT) && (auth_ctx->state != AUTH_CHAIN)) {
            auth_ctx->state = AUTH_FAILURE;
            PSA_ADAC_LOG_DEBUG("auth", "State inconsistent with receiving a certificate\r\n");
        } else if (fragment->length_in_bytes <= sizeof(certificate_header_t)) {
            auth_ctx->state = AUTH_FAILURE;
            PSA_ADAC_LOG_DEBUG("auth", "Size inconsistent with certificate\r\n");
        } else {
            PSA_ADAC_LOG_TRACE("auth", "Received a certificate\r\n");
            r = authentication_crt(auth_ctx, fragment->value, fragment->length_in_bytes);
        }
    } else if (fragment->type_id == PSA_BINARY_TOKEN) {
        if ((auth_ctx->state != AUTH_ROOT) && (auth_ctx->state != AUTH_LEAF)) {
            PSA_ADAC_LOG_DEBUG("auth", "State inconsistent with receiving a token\r\n");
        } else if (fragment->length_in_bytes <= sizeof(token_header_t)) {
            auth_ctx->state = AUTH_FAILURE;
            PSA_ADAC_LOG_DEBUG("auth", "Size inconsistent with token\r\n");
        } else {
            PSA_ADAC_LOG_TRACE("auth", "Received a token\r\n");
            r = authentication_token(auth_ctx, fragment->value, fragment->length_in_bytes);
        }
    } else {
        PSA_ADAC_LOG_WARN("auth", "Received neither a certificate nor a token\r\n");
    }

    (void) authenticator_request_packet_release(auth_ctx, request);
    return authenticator_response_packet_build(auth_ctx, r, NULL, 0);
}

void authentication_context_init(authentication_context_t *auth_ctx, uint8_t *buffer, size_t size,
                                 psa_algorithm_t rotpk_algo, uint8_t **rotpk, size_t *rotpk_size,
                                 uint8_t *rotpk_type, size_t rotpk_count) {
    for (size_t i = 0UL; i < sizeof(auth_ctx->permissions_mask); i++) {
        auth_ctx->permissions_mask[i] = 0xFF;
    }

    auth_ctx->rotpk_ctx.rotpk = rotpk;
    auth_ctx->rotpk_ctx.rotpk_algo = rotpk_algo;
    auth_ctx->rotpk_ctx.rotpk_size = rotpk_size;
    auth_ctx->rotpk_ctx.rotpk_type = rotpk_type;
    auth_ctx->rotpk_ctx.rotpk_count = rotpk_count;

    auth_ctx->context.content = buffer;
    auth_ctx->context.max = size;
    auth_ctx->context.size = 0;

    auth_ctx->state = AUTH_INIT;
}

static response_packet_t *authentication_change_lcs(authentication_context_t *auth_ctx, request_packet_t *request)
{
    adac_status_t status;
    (void) authenticator_request_packet_release(auth_ctx, request);
    psa_tlv_t *fragment = (psa_tlv_t *) &request->data;

    if (((request->data_count * 4) < sizeof(psa_tlv_t))
            || (((request->data_count * 4) - sizeof(psa_tlv_t)) < fragment->length_in_bytes)) {
        auth_ctx->state = AUTH_FAILURE;
        status = ADAC_INVALID_PARAMETERS;
    } else {
        status = psa_adac_change_life_cycle_state(fragment->value, fragment->length_in_bytes);
    }

    return authenticator_response_packet_build(auth_ctx, status, NULL, 0);
}

int authentication_handle(authentication_context_t *auth_ctx) {
    int done = 0;
    request_packet_t *request;

    PSA_ADAC_LOG_INFO("auth", "Starting authentication loop\r\n");
    while (done == 0) {
        request = authenticator_request_packet_receive(auth_ctx);
        if (NULL == request) {
            break;
        }

        PSA_ADAC_LOG_DEBUG("auth", "Receiving request %x\r\n", request->command);

        int ret;
        response_packet_t *response;
        switch (request->command) {
            case ADAC_DISCOVERY_CMD:
                response = authentication_discovery(auth_ctx, request);
                ret = authenticator_send_response(auth_ctx, response);
                break;

            case ADAC_AUTH_START_CMD:
                response = authentication_start(auth_ctx, request);
                ret = authenticator_send_response(auth_ctx, response);
                break;

            case ADAC_AUTH_RESPONSE_CMD:
                response = authentication_response(auth_ctx, request);
                ret = authenticator_send_response(auth_ctx, response);
                break;

                /* Send success status but otherwise do nothing. */
            case ADAC_LOCK_DEBUG_CMD:
                PSA_ADAC_LOG_DEBUG("auth", "Lock debug\r\n");
                (void) authenticator_request_packet_release(auth_ctx, request);
                psa_adac_platform_lock();
                response = authenticator_response_packet_build(auth_ctx, ADAC_SUCCESS, NULL, 0);
                ret = authenticator_send_response(auth_ctx, response);
                break;

                /* Send success status and terminate command loop. */
            case ADAC_RESUME_BOOT_CMD:
                PSA_ADAC_LOG_DEBUG("auth", "Resuming \"boot\"\r\n");
                (void) authenticator_request_packet_release(auth_ctx, request);
                response = authenticator_response_packet_build(auth_ctx, ADAC_SUCCESS, NULL, 0);
                ret = authenticator_send_response(auth_ctx, response);
                done = 1;
                break;

            case ADAC_LCS_CHANGE_CMD:
                PSA_ADAC_LOG_DEBUG("auth", "Change LCS \n");
                response = authentication_change_lcs(auth_ctx, request);
                ret = authenticator_send_response(auth_ctx, response);
                break;

            default:
                PSA_ADAC_LOG_ERR("auth", "Unknown command: %04x\r\n", request->command);
                (void) authenticator_request_packet_release(auth_ctx, request);
                response = authenticator_response_packet_build(auth_ctx, ADAC_INVALID_COMMAND, NULL, 0);
                ret = authenticator_send_response(auth_ctx, response);
                break;
        }

        if (ret != 0) {
            PSA_ADAC_LOG_ERR("auth", "Error sending response: %04x\r\n", ret);
        }

        if ((auth_ctx->state == AUTH_SUCCESS) || (auth_ctx->state == AUTH_FAILURE)) {
            done = 1;
        }
    }

    PSA_ADAC_LOG_INFO("auth", "Ending authentication loop\r\n");

    return done;
}
