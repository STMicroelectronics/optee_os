/*
 * Copyright (c) 2020 Arm Limited. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <psa_adac_config.h>

#define ROTPK_ANCHOR_ALG PSA_ALG_SHA_256

static uint8_t *rotpk_anchors[1];

static size_t rotpk_anchors_size[1];

static uint8_t rotpk_anchors_type[] = {
        ECDSA_P256_SHA256,
};

static size_t rotpk_anchors_length = sizeof(rotpk_anchors) / sizeof(uint8_t *);
