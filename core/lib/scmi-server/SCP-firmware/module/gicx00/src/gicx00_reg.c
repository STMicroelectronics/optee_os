/*
 * Arm SCP/MCP Software
 * Copyright (c) 2024-2025, Arm Limited and Contributors. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include "internal/gicx00_reg.h"

#include <arch_reg.h>

#define ICC_SRE_EL2     S3_4_C12_C9_5
#define ICC_IGRPEN0_el1 S3_0_C12_C12_6
#define ICC_PMR_EL1     S3_0_C4_C6_0

void write_icc_sre(uint64_t value)
{
    WRITE_SYSREG(ICC_SRE_EL2, value);
}

void write_igrpen0_el1(uint64_t value)
{
    WRITE_SYSREG(ICC_IGRPEN0_el1, value);
}

void write_icc_pmr(uint64_t value)
{
    WRITE_SYSREG(ICC_PMR_EL1, value);
}

uint64_t read_mpidr_el1(void)
{
    return READ_SYSREG(MPIDR_EL1);
}
