/*
 * Arm SCP/MCP Software
 * Copyright (c) 2024, Arm Limited and Contributors. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef INTERNAL_ARMV8R_MPU_H
#define INTERNAL_ARMV8R_MPU_H

#include <stdint.h>

uint64_t read_mair_el2(void);
void write_mair_el2(uint64_t);
uint64_t read_mpuir_el2(void);
void write_prbar_el2(uint64_t);
void write_prlar_el2(uint64_t);
void write_prselr_el2(uint64_t);
uint64_t read_sctlr_el2(void);
void write_sctlr_el2(uint64_t);

void barrier_dsync_fence_full(void);
void barrier_isync_fence_full(void);

#endif /* INTERNAL_ARMV8R_MPU_H */
