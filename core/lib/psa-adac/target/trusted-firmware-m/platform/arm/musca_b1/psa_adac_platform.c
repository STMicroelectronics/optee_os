/*
 * Copyright (c) 2023, Arm Limited. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 */

#include "psa_adac_config.h"
#include "psa_adac_debug.h"
#include "psa_adac_sda.h"
#include "platform/platform.h"
#include "platform/msg_interface.h"
/* Required for crypto_hw_apply_debug_permissions, the only API required
 * by ADAC which is not standardized through PSA Crypto but through the
 * TF-M specific crypto_hw.h header
 */
#include "crypto_hw.h"

#include <string.h>

#define ROTPK_ANCHOR_ALG PSA_ALG_SHA_256

extern uint8_t discovery_template[];
extern size_t discovery_template_len;

static uint8_t buffer[512];
static uint8_t messages[512];
static uint8_t *rotpk_anchors[1];
static size_t rotpk_anchors_size[1];
static uint8_t rotpk_anchors_type[] = {
               ECDSA_P256_SHA256,
};
static size_t rotpk_anchors_length = sizeof(rotpk_anchors) / sizeof(uint8_t *);

void psa_adac_platform_init(void)
{
    /* TODO: Code me */
}

size_t psa_adac_platform_discovery(uint8_t *reply, size_t reply_size)
{
    if (reply_size >= discovery_template_len) {
        memcpy(reply, discovery_template, discovery_template_len);
        return discovery_template_len;
    }
    return 0;
}

void psa_adac_platform_lock(void)
{
    /* TODO: Code me */
}

adac_status_t psa_adac_change_life_cycle_state(uint8_t *input, size_t input_size)
{
    /* TODO: Code me */
    /* LCS change is platform specific and is NOT implemented */
    /* Ignore return value and send UNSUPPORTED status for now */
    return ADAC_UNSUPPORTED;
}

int psa_adac_platform_check_token(uint8_t *token, size_t token_size)
{
    /* TODO: Code me */
    return 0;
}

int psa_adac_platform_check_certificate(uint8_t *crt, size_t crt_size)
{
    /* TODO: Code me */
    return 0;
}

int psa_adac_apply_permissions(uint8_t permissions_mask[16])
{
    int ret = crypto_hw_apply_debug_permissions(permissions_mask, 16);
    if (ret) {
        PSA_ADAC_LOG_ERR("platform", "psa_adac_to_tfm_apply_permissions "
                          "failed\r\n");
        return ret;
    }

    PSA_ADAC_LOG_INFO("platform",
                      "\r\nPlatform unlocked for the secure debug %s\r\n");
    return ret;
}

int tfm_to_psa_adac_musca_b1_secure_debug(uint8_t *secure_debug_roptpk, uint32_t len)
{
    authentication_context_t auth_ctx;
    int ret = -1;

    if (psa_adac_detect_debug_request()) {
        PSA_ADAC_LOG_INFO("main", "%s:%d Connection establised\r\n",
                          __func__, __LINE__);

        msg_interface_init(NULL, messages, sizeof(messages));

        psa_adac_init();
        psa_adac_acknowledge_debug_request();

        rotpk_anchors[0] = secure_debug_roptpk;
        rotpk_anchors_size[0] = len;
        authentication_context_init(&auth_ctx, buffer, sizeof(buffer),
                                    ROTPK_ANCHOR_ALG,
                                    rotpk_anchors, rotpk_anchors_size,
                                    rotpk_anchors_type,
                                    rotpk_anchors_length);
#ifndef PSA_ADAC_QUIET
        PSA_ADAC_LOG_INFO("main", "Starting authentication.\r\n");
#endif
        authentication_handle(&auth_ctx);

        PSA_ADAC_LOG_INFO("main", "\r\n\r\n\r\nAuthentication is a %s\r\n\r\n",
                auth_ctx.state == AUTH_SUCCESS ? "success" : "failure");

        if (auth_ctx.state == AUTH_SUCCESS) {
            ret = 0;
        }

        msg_interface_free(NULL);
    } else {
        PSA_ADAC_LOG_INFO("main", "%s:%d No secure debug connection.\r\n",
                          __func__, __LINE__);
    }

    return ret;
}

void platform_init(void)
{
    /* TODO: Code me */
}
