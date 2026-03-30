/*
 * Arm SCP/MCP Software
 * Copyright (c) 2024-2025, Arm Limited and Contributors. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <internal/armv8r_mpu_reg.h>

#include <arch_interrupt.h>
#include <arch_reg.h>

uint64_t read_mair_el2(void)
{
    return READ_SYSREG(mair_el2);
}

void write_mair_el2(uint64_t value)
{
    WRITE_SYSREG(mair_el2, value);
}

uint64_t read_mpuir_el2(void)
{
    return READ_SYSREG(mpuir_el2);
}

void write_prbar_el2(uint64_t value)
{
    WRITE_SYSREG(prbar_el2, value);
}

void write_prlar_el2(uint64_t value)
{
    WRITE_SYSREG(prlar_el2, value);
}

void write_prselr_el2(uint64_t value)
{
    WRITE_SYSREG(prselr_el2, value);
}

uint64_t read_sctlr_el2(void)
{
    return READ_SYSREG(sctlr_el2);
}

void write_sctlr_el2(uint64_t value)
{
    WRITE_SYSREG(sctlr_el2, value);
}

void barrier_dsync_fence_full(void)
{
    BARRIER_DSYNC_FENCE_FULL();
}

void barrier_isync_fence_full(void)
{
    BARRIER_ISYNC_FENCE_FULL();
}
