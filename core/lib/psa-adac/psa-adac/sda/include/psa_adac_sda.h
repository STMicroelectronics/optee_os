/*
 * Copyright (c) 2020 Arm Limited. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef PSA_ADAC_SDA_H
#define PSA_ADAC_SDA_H

#include <psa/crypto.h>
#include <psa_adac.h>

/** \addtogroup adac-sda
 * @{
 */

typedef struct {
    psa_algorithm_t rotpk_algo;
    uint8_t **rotpk;
    size_t *rotpk_size;
    uint8_t *rotpk_type;
    size_t rotpk_count;
} rotpk_context_t;

typedef enum {
    AUTH_INIT,
    AUTH_CHALLENGE,
    AUTH_ROT_META,
    AUTH_ROOT,
    AUTH_CHAIN,
    AUTH_LEAF,
    AUTH_TOKEN,
    AUTH_SUCCESS,
    AUTH_FAILURE
} authentication_state_t;

#include <platform/msg_interface.h>

typedef struct {
    uint8_t permissions_mask[16];
    psa_auth_challenge_t challenge;
    rotpk_context_t rotpk_ctx;
    validation_context_t context;
    authentication_state_t state;
#ifndef PSA_ADAC_AUTHENTICATOR_IMPLICIT_TRANSPORT
    target_msg_interface_t msg_interface;
    void *msg_ctx;
#endif
} authentication_context_t;

/** \brief Initialize authentication context
 */
void authentication_context_init(authentication_context_t *auth_ctx, uint8_t *buffer, size_t size,
                                 psa_algorithm_t rotpk_algo, uint8_t **rotpk, size_t *rotpk_size,
                                 uint8_t *rotpk_type, size_t rotpk_count);

/**
 */
response_packet_t *authentication_discovery(authentication_context_t *auth_ctx, request_packet_t *request);

/**
 */
response_packet_t *authentication_start(authentication_context_t *auth_ctx, request_packet_t *request);

/**
 */
response_packet_t *authentication_response(authentication_context_t *auth_ctx, request_packet_t *request);

#ifndef PSA_ADAC_AUTHENTICATOR_IMPLICIT_TRANSPORT
void authentication_context_set_transport(authentication_context_t *auth_ctx,
                                          target_msg_interface_t msg_interface, void *msg_ctx);
#endif

/**
 */
int authentication_handle(authentication_context_t *auth_ctx);

/**@}*/

#endif //PSA_ADAC_SDA_H
