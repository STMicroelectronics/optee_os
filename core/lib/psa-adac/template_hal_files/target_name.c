/*
 * Copyright (c) 2020-2023 Arm Limited. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include "psa_adac_config.h"
#include "psa_adac_debug.h"
#include "psa_adac_sda.h"
#include "platform/platform.h"
#include "platform/msg_interface.h"

void psa_adac_platform_init(void)
{
    /* TODO: Code me */
}

size_t psa_adac_platform_discovery(uint8_t *reply, size_t reply_size)
{
    /* TODO: Code me */
    return 0;
}

adac_status_t psa_adac_change_life_cycle_state(uint8_t *input, size_t input_size)
{
    /* TODO: Code me */
    return ADAC_UNSUPPORTED;
}

void psa_adac_platform_lock(void)
{
    /* TODO: Code me */
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
    /* TODO: Code me */
    return 0;
}

void platform_init(void)
{
    /* TODO: Code me */
}
