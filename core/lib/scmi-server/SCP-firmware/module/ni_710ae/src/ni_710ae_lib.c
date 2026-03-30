/*
 * Arm SCP/MCP Software
 * Copyright (c) 2025, Arm Limited and Contributors. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 */

#include "internal/ni_710ae_lib.h"

#include <stddef.h>

int program_ni_710ae_apu(
    const struct ni_710ae_component_apu_config *config,
    uint16_t apu_config_count,
    uintptr_t dev_periphbase_addr,
    struct ni710ae_discovery_tree_t *discovery_tree)
{
    int err = FWK_SUCCESS;

    for (uint16_t apu_idx = 0; apu_idx < apu_config_count; apu_idx++) {
        struct ni710ae_apu_dev_t apu_dev = {
            .base = dev_periphbase_addr +
                ni710ae_fetch_offset_address(
                        discovery_tree,
                        config[apu_idx].component_type,
                        config[apu_idx].component_id,
                        NI710AE_NODE_TYPE_SUBFEATURE_APU),
        };

        const uint16_t apu_subregion_count =
            config[apu_idx].apu_subregion_count;

        for (uint16_t subregion_idx = 0; subregion_idx < apu_subregion_count;
             subregion_idx++) {
            const struct ni_710ae_apu_subregion_configs *r_map =
                &config[apu_idx].regions[subregion_idx];

            uint8_t entity_ids[NCI_MAX_NUMBER_OF_ID] = {
                NCI_ID_0_SELECT,
                NCI_ID_UNASSIGNED,
                NCI_ID_UNASSIGNED,
                NCI_ID_UNASSIGNED,
            };

            uint8_t entity_permissions[NCI_MAX_NUMBER_OF_ID] = {
                r_map->perms,
                NCI_ID_PERMISSION_UNASSIGNED,
                NCI_ID_PERMISSION_UNASSIGNED,
                NCI_ID_PERMISSION_UNASSIGNED,
            };

            const struct ni710ae_apu_region region_cfg = {
                .region = r_map->region,
                .base_addr = r_map->base_addr,
                .end_addr = r_map->end_addr,
                .background = r_map->br,
                .permissions = entity_permissions,
                .entity_ids = entity_ids,
                .id_valid = NCI_ID_VALID_NONE,
                .region_enable = NCI_REGION_ENABLE,
                .lock = NCI_UNLOCK,
            };

            err = ni710ae_apu_initialize_region(&apu_dev, &region_cfg);
            if (err != FWK_SUCCESS) {
                return err;
            }
        }

        err = ni710ae_apu_sync_err_enable(&apu_dev);
        if (err != FWK_SUCCESS) {
            return err;
        }

        err = ni710ae_apu_enable(&apu_dev);
        if (err != FWK_SUCCESS) {
            return err;
        }
    }

    return err;
}
