/*
 * Arm SCP/MCP Software
 * Copyright (c) 2024, Arm Limited and Contributors. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 * Description:
 *      Metrics Analyzer unit test support.
 */

#ifndef MOD_METRICS_ANALYZER_EXTRA_H
#define MOD_METRICS_ANALYZER_EXTRA_H

#include <fwk_id.h>

int get_power_limit(fwk_id_t id, uint32_t *power_limit);
int set_power_limit(fwk_id_t id, uint32_t power_limit);

#endif /* MOD_METRICS_ANALYZER_EXTRA_H */
