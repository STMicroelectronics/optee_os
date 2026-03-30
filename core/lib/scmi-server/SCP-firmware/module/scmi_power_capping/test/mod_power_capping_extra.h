/*
 * Arm SCP/MCP Software
 * Copyright (c) 2023-2025, Arm Limited and Contributors. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef MOD_POWER_CAPPING_EXTRA_H_
#define MOD_POWER_CAPPING_EXTRA_H_

#include "fwk_id.h"
#include "mod_power_capping.h"

int get_applied_cap(fwk_id_t domain_id, uint32_t *cap);
int request_cap(fwk_id_t domain_id, uint32_t cap);
int get_average_power(fwk_id_t id, uint32_t *power);
int set_averaging_interval(fwk_id_t id, uint32_t pai);
int get_averaging_interval(fwk_id_t id, uint32_t *pai);
int get_averaging_interval_step(fwk_id_t id, uint32_t *pai_step);
int get_averaging_interval_range(
    fwk_id_t id,
    uint32_t *min_pai,
    uint32_t *max_pai);
int set_power_thresholds(
    fwk_id_t domain_id,
    uint32_t threshold_low,
    uint32_t threshold_high);
#endif /* MOD_POWER_CAPPING_EXTRA_H_ */
