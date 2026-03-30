/*
 * Arm SCP/MCP Software
 * Copyright (c) 2024, Arm Limited and Contributors. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 * Description:
 *      Power unit test support.
 */

#ifndef MOD_POWER_DISTRIBUTOR_EXTRA_H
#define MOD_POWER_DISTRIBUTOR_EXTRA_H

#include <fwk_id.h>

int mock_get_power_limit(fwk_id_t id, uint32_t *power_limit);
int mock_set_power_limit(fwk_id_t id, uint32_t power_limit);
int mock_set_power_demand(fwk_id_t id, uint32_t power_demand);

#endif /* MOD_POWER_DISTRIBUTOR_EXTRA_H */
