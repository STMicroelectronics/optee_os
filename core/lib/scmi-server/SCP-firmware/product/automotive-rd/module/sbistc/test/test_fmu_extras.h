/*
 * Arm SCP/MCP Software
 * Copyright (c) 2025, Arm Limited and Contributors. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 */

#ifndef TEST_FMU_EXTRAS_H
#define TEST_FMU_EXTRAS_H

#include <mod_fmu.h>

int test_fmu_set_enabled(const struct mod_fmu_fault *fault, bool enable);
int test_fmu_get_count(fwk_id_t id, uint16_t node, uint8_t *count);

#endif /* TEST_FMU_EXTRAS_H */
