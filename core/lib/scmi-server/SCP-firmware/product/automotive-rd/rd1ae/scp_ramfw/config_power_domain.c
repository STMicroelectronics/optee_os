/*
 * Arm SCP/MCP Software
 * Copyright (c) 2024, Arm Limited and Contributors. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 * Description:
 *     Configuration data for module 'power_domain'.
 */

#include "platform_core.h"
#include "scp_cfgd_power_domain.h"

#include <power_domain_utils.h>

#include <mod_power_domain.h>
#include <mod_ppu_v1.h>
#include <mod_system_power.h>

#include <fwk_element.h>
#include <fwk_id.h>
#include <fwk_macros.h>
#include <fwk_mm.h>
#include <fwk_module.h>
#include <fwk_module_idx.h>
#include <fwk_string.h>

#include <stddef.h>

#define PD_STATIC_ELEMENT_COUNT (PD_STATIC_DEV_IDX_SYSTOP + 1)

/* Allowed PD state count */
#define SYSTOP_PD_STATE_COUNT 1
#define CLUS_PD_STATE_COUNT   2
#define CORE_PD_STATE_COUNT   2

/* Mask for the cluster valid power states */
#define CLUSTER_VALID_STATE_MASK (MOD_PD_STATE_OFF_MASK | MOD_PD_STATE_ON_MASK)

/* Mask for the core valid power states */
#define CORE_VALID_STATE_MASK (MOD_PD_STATE_OFF_MASK | MOD_PD_STATE_ON_MASK)

/* Mask of the allowed states for the systop power domain */
static const uint32_t systop_allowed_state_mask_table[SYSTOP_PD_STATE_COUNT] = {
    [0] = MOD_PD_STATE_ON_MASK
};

/*
 * Mask of the allowed states for the cluster power domain depending on the
 * system states.
 */
static const uint32_t
    cluster_pd_allowed_state_mask_table[CLUS_PD_STATE_COUNT] = {
        [MOD_PD_STATE_OFF] = MOD_PD_STATE_OFF_MASK,
        [MOD_PD_STATE_ON] = CLUSTER_VALID_STATE_MASK,
    };

/* Mask of the allowed states for a core depending on the cluster states. */
static const uint32_t core_pd_allowed_state_mask_table[CORE_PD_STATE_COUNT] = {
    [MOD_PD_STATE_OFF] = MOD_PD_STATE_OFF_MASK | MOD_PD_STATE_SLEEP_MASK,
    [MOD_PD_STATE_ON] = CORE_VALID_STATE_MASK,
};

/* Power module specific configuration data (none) */
static const struct mod_power_domain_config platform_power_domain_config = {
    0
};

static struct fwk_element pd_static_element_table[PD_STATIC_ELEMENT_COUNT] = {
    [PD_STATIC_DEV_IDX_SYSTOP] = {
        .name = "SYSTOP",
        .data = &((struct mod_power_domain_element_config) {
            .attributes.pd_type = MOD_PD_TYPE_SYSTEM,
            .parent_idx = PD_STATIC_DEV_IDX_NONE,
            .driver_id = FWK_ID_MODULE_INIT(FWK_MODULE_IDX_SYSTEM_POWER),
            .api_id = FWK_ID_API_INIT(
                FWK_MODULE_IDX_SYSTEM_POWER,
                MOD_SYSTEM_POWER_API_IDX_PD_DRIVER),
            .allowed_state_mask_table = systop_allowed_state_mask_table,
            .allowed_state_mask_table_size =
                FWK_ARRAY_SIZE(systop_allowed_state_mask_table)
        }),
    },
};

static const struct fwk_element *platform_power_domain_get_element_table(
    fwk_id_t module_id)
{
    const struct fwk_element *systop_elements = NULL;
    const struct fwk_element *si_clus0_elements = NULL;
    const struct fwk_element *si_clus1_elements = NULL;
    const struct fwk_element *si_clus2_elements = NULL;
    struct fwk_element *all_elements = NULL;
    struct fwk_element tmp_table[1] = { 0 };
    unsigned int element_idx;
    struct mod_power_domain_element_config *pd_config;
    unsigned int systop_elements_count, si_clus0_elements_count,
        si_clus1_elements_count, si_clus2_elements_count;

    /* Create power doamin elements for SYSTOP */
    systop_elements = create_power_domain_element_table(
        platform_get_core_count(),
        platform_get_cluster_count(),
        FWK_MODULE_IDX_PPU_V1,
        MOD_PPU_V1_API_IDX_POWER_DOMAIN_DRIVER,
        core_pd_allowed_state_mask_table,
        FWK_ARRAY_SIZE(core_pd_allowed_state_mask_table),
        cluster_pd_allowed_state_mask_table,
        FWK_ARRAY_SIZE(cluster_pd_allowed_state_mask_table),
        pd_static_element_table,
        FWK_ARRAY_SIZE(pd_static_element_table));

    if (systop_elements == NULL) {
        return NULL;
    }

    /* Create power domain elements for SYSTOP_SI.
     * Now 7 core in 3 cluster is supported.
     */
    si_clus0_elements = create_power_domain_element_table(
        1,
        1,
        FWK_MODULE_IDX_PPU_V1,
        MOD_PPU_V1_API_IDX_POWER_DOMAIN_DRIVER,
        core_pd_allowed_state_mask_table,
        FWK_ARRAY_SIZE(core_pd_allowed_state_mask_table),
        cluster_pd_allowed_state_mask_table,
        FWK_ARRAY_SIZE(cluster_pd_allowed_state_mask_table),
        tmp_table,
        0);

    if (si_clus0_elements == NULL) {
        return NULL;
    }

    si_clus1_elements = create_power_domain_element_table(
        2,
        1,
        FWK_MODULE_IDX_PPU_V1,
        MOD_PPU_V1_API_IDX_POWER_DOMAIN_DRIVER,
        core_pd_allowed_state_mask_table,
        FWK_ARRAY_SIZE(core_pd_allowed_state_mask_table),
        cluster_pd_allowed_state_mask_table,
        FWK_ARRAY_SIZE(cluster_pd_allowed_state_mask_table),
        tmp_table,
        0);

    if (si_clus1_elements == NULL) {
        return NULL;
    }

    si_clus2_elements = create_power_domain_element_table(
        4,
        1,
        FWK_MODULE_IDX_PPU_V1,
        MOD_PPU_V1_API_IDX_POWER_DOMAIN_DRIVER,
        core_pd_allowed_state_mask_table,
        FWK_ARRAY_SIZE(core_pd_allowed_state_mask_table),
        cluster_pd_allowed_state_mask_table,
        FWK_ARRAY_SIZE(cluster_pd_allowed_state_mask_table),
        tmp_table,
        0);

    if (si_clus2_elements == NULL) {
        return NULL;
    }

    systop_elements_count = platform_get_core_count() +
        platform_get_cluster_count() + FWK_ARRAY_SIZE(pd_static_element_table);

    si_clus0_elements_count = 2;
    si_clus1_elements_count = 3;
    si_clus2_elements_count = 5;
    /* Create an array for all elements */
    all_elements = fwk_mm_calloc(
        systop_elements_count + si_clus0_elements_count +
            si_clus1_elements_count + si_clus2_elements_count + 1,
        sizeof(struct fwk_element));

    if (all_elements == NULL) {
        return NULL;
    }

    /* Copy SYSTOP elements to the new array */
    fwk_str_memcpy(
        all_elements,
        systop_elements,
        systop_elements_count * sizeof(struct fwk_element));
    /* Copy the SI_clus0 elements to the new array.
     * SI elements follow SYSTOP elements. */
    fwk_str_memcpy(
        all_elements + systop_elements_count,
        si_clus0_elements,
        si_clus0_elements_count * sizeof(struct fwk_element));
    /* Copy the SI_clus1 elements to the new array.
     * SI elements follow SYSTOP elements. */
    fwk_str_memcpy(
        all_elements + systop_elements_count + si_clus0_elements_count,
        si_clus1_elements,
        si_clus1_elements_count * sizeof(struct fwk_element));
    /* Copy the SI_clus2 elements to the new array.
     * SI elements follow SYSTOP elements. */
    fwk_str_memcpy(
        all_elements + systop_elements_count + si_clus0_elements_count +
            si_clus1_elements_count,
        si_clus2_elements,
        si_clus2_elements_count * sizeof(struct fwk_element));

    /* The parent indices of SI elements need to be updated
     * because they are in a new array, their indices are not zero based. */

    /* Update CL0CORE0 */
    element_idx = systop_elements_count;
    pd_config =
        (struct mod_power_domain_element_config *)all_elements[element_idx]
            .data;
    pd_config->parent_idx = element_idx + 1;
    pd_config->driver_id = FWK_ID_ELEMENT(FWK_MODULE_IDX_PPU_V1, element_idx);

    /* Update CL0 */
    element_idx = element_idx + 1;
    pd_config =
        (struct mod_power_domain_element_config *)all_elements[element_idx]
            .data;
    pd_config->parent_idx = PD_STATIC_DEV_IDX_NONE;
    pd_config->driver_id = FWK_ID_ELEMENT(FWK_MODULE_IDX_PPU_V1, element_idx);

    /* Update CL1CORE0 */
    element_idx = systop_elements_count + 2;
    pd_config =
        (struct mod_power_domain_element_config *)all_elements[element_idx]
            .data;
    pd_config->parent_idx = systop_elements_count + 4;
    pd_config->driver_id = FWK_ID_ELEMENT(FWK_MODULE_IDX_PPU_V1, element_idx);

    /* Update CL1CORE1 */
    element_idx = systop_elements_count + 3;
    pd_config =
        (struct mod_power_domain_element_config *)all_elements[element_idx]
            .data;
    pd_config->parent_idx = systop_elements_count + 4;
    pd_config->driver_id = FWK_ID_ELEMENT(FWK_MODULE_IDX_PPU_V1, element_idx);

    /* Update CL1 */
    element_idx = systop_elements_count + 4;
    pd_config =
        (struct mod_power_domain_element_config *)all_elements[element_idx]
            .data;
    pd_config->parent_idx = PD_STATIC_DEV_IDX_NONE;
    pd_config->driver_id = FWK_ID_ELEMENT(FWK_MODULE_IDX_PPU_V1, element_idx);

    /* Update CL2CORE0 */
    element_idx = systop_elements_count + 5;
    pd_config =
        (struct mod_power_domain_element_config *)all_elements[element_idx]
            .data;
    pd_config->parent_idx = systop_elements_count + 9;
    pd_config->driver_id = FWK_ID_ELEMENT(FWK_MODULE_IDX_PPU_V1, element_idx);

    /* Update CL2CORE1 */
    element_idx = systop_elements_count + 6;
    pd_config =
        (struct mod_power_domain_element_config *)all_elements[element_idx]
            .data;
    pd_config->parent_idx = systop_elements_count + 9;
    pd_config->driver_id = FWK_ID_ELEMENT(FWK_MODULE_IDX_PPU_V1, element_idx);

    /* Update CL2CORE2 */
    element_idx = systop_elements_count + 7;
    pd_config =
        (struct mod_power_domain_element_config *)all_elements[element_idx]
            .data;
    pd_config->parent_idx = systop_elements_count + 9;
    pd_config->driver_id = FWK_ID_ELEMENT(FWK_MODULE_IDX_PPU_V1, element_idx);

    /* Update CL2CORE3 */
    element_idx = systop_elements_count + 8;
    pd_config =
        (struct mod_power_domain_element_config *)all_elements[element_idx]
            .data;
    pd_config->parent_idx = systop_elements_count + 9;
    pd_config->driver_id = FWK_ID_ELEMENT(FWK_MODULE_IDX_PPU_V1, element_idx);

    /* Update CL2 */
    element_idx = systop_elements_count + 9;
    pd_config =
        (struct mod_power_domain_element_config *)all_elements[element_idx]
            .data;
    pd_config->parent_idx = PD_STATIC_DEV_IDX_NONE;
    pd_config->driver_id = FWK_ID_ELEMENT(FWK_MODULE_IDX_PPU_V1, element_idx);

    return all_elements;
}

const struct fwk_module_config config_power_domain = {
    .data = &platform_power_domain_config,
    .elements =
        FWK_MODULE_DYNAMIC_ELEMENTS(platform_power_domain_get_element_table),
};
