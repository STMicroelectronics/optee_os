/*
 * Arm SCP/MCP Software
 * Copyright (c) 2024-2025, Arm Limited and Contributors. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 */

#include "fwk_event.h"

int get_power_limit(fwk_id_t id, uint32_t *power_limit);

int get_average_power(fwk_id_t id, uint32_t *power);

int set_averaging_interval(fwk_id_t id, uint32_t pai);

int get_averaging_interval(fwk_id_t id, uint32_t *pai);

int get_averaging_interval_step(fwk_id_t id, uint32_t *pai_step);

int get_averaging_interval_range(
    fwk_id_t id,
    uint32_t *min_pai,
    uint32_t *max_pai);

int update(fwk_id_t id, int64_t input, int64_t *output);

int set_point(fwk_id_t id, int64_t input);
