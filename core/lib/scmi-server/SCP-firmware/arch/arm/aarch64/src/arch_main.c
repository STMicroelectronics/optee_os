/*
 * Arm SCP/MCP Software
 * Copyright (c) 2024-2025, Arm Limited and Contributors. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <fwk_arch.h>
#include <fwk_assert.h>
#include <fwk_macros.h>
#include <fwk_status.h>

#include <arch_interrupt.h>

FWK_WEAK int platform_init_hook(void *params)
{
    return FWK_SUCCESS;
}

int arm_main(void)
{
    int status;

    status = platform_init_hook(NULL);
    if (status != FWK_SUCCESS) {
        fwk_trap();
    }

    return fwk_arch_init();
}
