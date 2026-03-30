/*
 * Arm SCP/MCP Software
 * Copyright (c) 2025, Arm Limited and Contributors. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 * Description:
 *      ATU MMIO unit test support.
 */

#include <mod_atu.h>

/* Mock atu_add_region function */
int mock_atu_add_region(
    const struct atu_region_map *region,
    fwk_id_t atu_device_id,
    uint8_t *region_idx);

/* Mock atu_remove_region function */
int mock_atu_remove_region(
    uint8_t region_idx,
    fwk_id_t atu_device_id,
    fwk_id_t requester_id);
