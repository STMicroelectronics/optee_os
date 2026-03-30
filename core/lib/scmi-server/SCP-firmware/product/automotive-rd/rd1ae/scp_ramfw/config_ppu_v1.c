/*
 * Arm SCP/MCP Software
 * Copyright (c) 2024, Arm Limited and Contributors. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 * Description:
 *     Configuration data for module 'ppu_v1'.
 */

#include "platform_core.h"
#include "scp_cfgd_power_domain.h"
#include "scp_mmap.h"

#include <mod_power_domain.h>
#include <mod_ppu_v1.h>

#include <fwk_element.h>
#include <fwk_id.h>
#include <fwk_interrupt.h>
#include <fwk_macros.h>
#include <fwk_mm.h>
#include <fwk_module.h>
#include <fwk_module_idx.h>
#include <fwk_string.h>

#include <stdio.h>

#define PPU_STATIC_ELEMENT_COUNT 1
#define PPU_CORE_NAME_SIZE       12
#define PPU_CLUS_NAME_SIZE       7

/* Safety Island CPU PPU base addresses */
#define SCP_PPU_SI_CLUS0      (0x56010000UL)
#define SCP_PPU_SI_CLUS0CORE0 (0x56040000UL)
#define SCP_PPU_SI_CLUS1      (0x56410000UL)
#define SCP_PPU_SI_CLUS1CORE0 (0x56440000UL)
#define SCP_PPU_SI_CLUS1CORE1 (0x56540000UL)
#define SCP_PPU_SI_CLUS2      (0x56810000UL)
#define SCP_PPU_SI_CLUS2CORE0 (0x56840000UL)
#define SCP_PPU_SI_CLUS2CORE1 (0x56940000UL)
#define SCP_PPU_SI_CLUS2CORE2 (0x56A40000UL)
#define SCP_PPU_SI_CLUS2CORE3 (0x56B40000UL)

/* Module configuration data */
static struct mod_ppu_v1_config ppu_v1_config_data = {
    .pd_notification_id = FWK_ID_NOTIFICATION_INIT(
        FWK_MODULE_IDX_POWER_DOMAIN,
        MOD_PD_NOTIFICATION_IDX_POWER_STATE_TRANSITION),
};

/* List of static PPU elements */
static struct fwk_element ppu_element_table[PPU_STATIC_ELEMENT_COUNT] = {
    {
        .name = "SYS0",
        .data = &((struct mod_ppu_v1_pd_config){
            .pd_type = MOD_PD_TYPE_SYSTEM,
            .ppu.reg_base = SCP_PPU_SYS0_BASE,
            .default_power_on = true,
            .observer_id = FWK_ID_NONE_INIT,
        }),
    },
};

static const struct fwk_element *ppu_v1_get_element_table(fwk_id_t module_id)
{
    struct fwk_element *element_table;
    struct mod_ppu_v1_pd_config *pd_config_table;
    unsigned int cluster_idx;
    unsigned int core_count;
    unsigned int cluster_count;
    unsigned int core_element_count = 0;
    unsigned int number_elements;
    int snprintf_ret_val;

    core_count = platform_get_core_count();
    cluster_count = platform_get_cluster_count();

    /*
     * Allocate element descriptors based on:
     *   Number of cores
     *   + Number of cluster descriptors
     *   + Number of system power domain descriptors
     *   + Number of SI power domain descriptors
     *     (10 for now: cluster0-core0, cluster0
     *                  cluster1-core0,1, cluster1
     *                  cluster2-core0-3, cluster2)
     *   + 1 terminator descriptor
     */
    number_elements =
        core_count + cluster_count + FWK_ARRAY_SIZE(ppu_element_table) + 10 + 1;
    element_table = fwk_mm_calloc(number_elements, sizeof(struct fwk_element));

    pd_config_table = fwk_mm_calloc(
        core_count + cluster_count, sizeof(struct mod_ppu_v1_pd_config));

    for (cluster_idx = 0; cluster_idx < cluster_count; cluster_idx++) {
        struct fwk_element *element;
        struct mod_ppu_v1_pd_config *pd_config;
        unsigned int core_idx;

        for (core_idx = 0;
             core_idx < platform_get_core_per_cluster_count(cluster_idx);
             core_idx++) {
            element = &element_table[core_element_count];
            pd_config = &pd_config_table[core_element_count];

            element->name = fwk_mm_alloc(PPU_CORE_NAME_SIZE, 1);

            snprintf_ret_val = snprintf(
                (char *)element->name,
                PPU_CORE_NAME_SIZE,
                "CLUS%uCORE%u",
                cluster_idx,
                core_idx);

            fwk_assert(
                (snprintf_ret_val >= 0) &&
                (snprintf_ret_val <= PPU_CORE_NAME_SIZE));

            element->data = pd_config;

            pd_config->pd_type = MOD_PD_TYPE_CORE;
            pd_config->ppu.reg_base =
                SCP_CLUSTER_UTILITY_CORE_PPU_BASE(cluster_idx);
            pd_config->ppu.irq = FWK_INTERRUPT_NONE;
            pd_config->cluster_id = FWK_ID_ELEMENT(
                FWK_MODULE_IDX_PPU_V1, (core_count + cluster_idx));
            pd_config->observer_id = FWK_ID_NONE;
            core_element_count++;
        }

        element = &element_table[core_count + cluster_idx];
        pd_config = &pd_config_table[core_count + cluster_idx];

        element->name = fwk_mm_alloc(PPU_CLUS_NAME_SIZE, 1);

        snprintf_ret_val = snprintf(
            (char *)element->name, PPU_CLUS_NAME_SIZE, "CLUS%u", cluster_idx);

        fwk_assert(
            (snprintf_ret_val >= 0) &&
            (snprintf_ret_val <= PPU_CLUS_NAME_SIZE));

        element->data = pd_config;

        pd_config->pd_type = MOD_PD_TYPE_CLUSTER;
        pd_config->ppu.irq = FWK_INTERRUPT_NONE;
        pd_config->observer_id = FWK_ID_NONE;
        pd_config->observer_api = FWK_ID_NONE;
        pd_config->ppu.reg_base =
            SCP_CLUSTER_UTILITY_CLUSTER_PPU_BASE(cluster_idx);
    }

    fwk_str_memcpy(
        &element_table[core_count + cluster_count],
        ppu_element_table,
        sizeof(ppu_element_table));

    /* Setting SI elements */
    /* Allocate pd_config for SI elements */
    pd_config_table = fwk_mm_calloc(10, sizeof(struct mod_ppu_v1_pd_config));

    /* SI Cluster 0 Core 0 */
    pd_config_table[0].pd_type = MOD_PD_TYPE_CORE;
    pd_config_table[0].ppu.reg_base = SCP_PPU_SI_CLUS0CORE0;
    pd_config_table[0].ppu.irq = FWK_INTERRUPT_NONE;
    /* 8 cores + 8 clusters + 1 systop + 1 si-core */
    pd_config_table[0].cluster_id = FWK_ID_ELEMENT(
        FWK_MODULE_IDX_PPU_V1, (core_count + cluster_count + 1 + 1));
    pd_config_table[0].observer_id = FWK_ID_NONE;
    element_table[core_count + cluster_count + 1].name = "CLUS0CORE0";
    element_table[core_count + cluster_count + 1].data = &pd_config_table[0];

    /* SI Cluster 0 */
    pd_config_table[1].pd_type = MOD_PD_TYPE_CLUSTER;
    pd_config_table[1].ppu.reg_base = SCP_PPU_SI_CLUS0;
    pd_config_table[1].ppu.irq = FWK_INTERRUPT_NONE;
    pd_config_table[1].observer_id = FWK_ID_NONE;
    pd_config_table[1].observer_api = FWK_ID_NONE;
    element_table[core_count + cluster_count + 2].name = "CLUS0";
    element_table[core_count + cluster_count + 2].data = &pd_config_table[1];

    /* SI Cluster 1 Core 0 */
    pd_config_table[2].pd_type = MOD_PD_TYPE_CORE;
    pd_config_table[2].ppu.reg_base = SCP_PPU_SI_CLUS1CORE0;
    pd_config_table[2].ppu.irq = FWK_INTERRUPT_NONE;
    pd_config_table[2].cluster_id = FWK_ID_ELEMENT(
        FWK_MODULE_IDX_PPU_V1, (core_count + cluster_count + 1 + 2 + 2));
    pd_config_table[2].observer_id = FWK_ID_NONE;
    element_table[core_count + cluster_count + 3].name = "CLUS1CORE0";
    element_table[core_count + cluster_count + 3].data = &pd_config_table[2];

    /* SI Cluster 1 Core 1 */
    pd_config_table[3].pd_type = MOD_PD_TYPE_CORE;
    pd_config_table[3].ppu.reg_base = SCP_PPU_SI_CLUS1CORE1;
    pd_config_table[3].ppu.irq = FWK_INTERRUPT_NONE;
    pd_config_table[3].cluster_id = FWK_ID_ELEMENT(
        FWK_MODULE_IDX_PPU_V1, (core_count + cluster_count + 1 + 2 + 2));
    pd_config_table[3].observer_id = FWK_ID_NONE;
    element_table[core_count + cluster_count + 4].name = "CLUS1CORE1";
    element_table[core_count + cluster_count + 4].data = &pd_config_table[3];

    /* SI Cluster 1 */
    pd_config_table[4].pd_type = MOD_PD_TYPE_CLUSTER;
    pd_config_table[4].ppu.reg_base = SCP_PPU_SI_CLUS1;
    pd_config_table[4].ppu.irq = FWK_INTERRUPT_NONE;
    pd_config_table[4].observer_id = FWK_ID_NONE;
    pd_config_table[4].observer_api = FWK_ID_NONE;
    element_table[core_count + cluster_count + 5].name = "CLUS1";
    element_table[core_count + cluster_count + 5].data = &pd_config_table[4];

    /* SI Cluster 2 Core 0 */
    pd_config_table[5].pd_type = MOD_PD_TYPE_CORE;
    pd_config_table[5].ppu.reg_base = SCP_PPU_SI_CLUS2CORE0;
    pd_config_table[5].ppu.irq = FWK_INTERRUPT_NONE;
    pd_config_table[5].cluster_id = FWK_ID_ELEMENT(
        FWK_MODULE_IDX_PPU_V1, (core_count + cluster_count + 1 + 2 + 3 + 4));
    pd_config_table[5].observer_id = FWK_ID_NONE;
    element_table[core_count + cluster_count + 6].name = "CLUS2CORE0";
    element_table[core_count + cluster_count + 6].data = &pd_config_table[5];

    /* SI Cluster 2 Core 1 */
    pd_config_table[6].pd_type = MOD_PD_TYPE_CORE;
    pd_config_table[6].ppu.reg_base = SCP_PPU_SI_CLUS2CORE1;
    pd_config_table[6].ppu.irq = FWK_INTERRUPT_NONE;
    pd_config_table[6].cluster_id = FWK_ID_ELEMENT(
        FWK_MODULE_IDX_PPU_V1, (core_count + cluster_count + 1 + 2 + 3 + 4));
    pd_config_table[6].observer_id = FWK_ID_NONE;
    element_table[core_count + cluster_count + 7].name = "CLUS2CORE1";
    element_table[core_count + cluster_count + 7].data = &pd_config_table[6];

    /* SI Cluster 2 Core 2 */
    pd_config_table[7].pd_type = MOD_PD_TYPE_CORE;
    pd_config_table[7].ppu.reg_base = SCP_PPU_SI_CLUS2CORE2;
    pd_config_table[7].ppu.irq = FWK_INTERRUPT_NONE;
    pd_config_table[7].cluster_id = FWK_ID_ELEMENT(
        FWK_MODULE_IDX_PPU_V1, (core_count + cluster_count + 1 + 2 + 3 + 4));
    pd_config_table[7].observer_id = FWK_ID_NONE;
    element_table[core_count + cluster_count + 8].name = "CLUS2CORE2";
    element_table[core_count + cluster_count + 8].data = &pd_config_table[7];

    /* SI Cluster 2 Core 3 */
    pd_config_table[8].pd_type = MOD_PD_TYPE_CORE;
    pd_config_table[8].ppu.reg_base = SCP_PPU_SI_CLUS2CORE3;
    pd_config_table[8].ppu.irq = FWK_INTERRUPT_NONE;
    pd_config_table[8].cluster_id = FWK_ID_ELEMENT(
        FWK_MODULE_IDX_PPU_V1, (core_count + cluster_count + 1 + 2 + 3 + 4));
    pd_config_table[8].observer_id = FWK_ID_NONE;
    element_table[core_count + cluster_count + 9].name = "CLUS2CORE3";
    element_table[core_count + cluster_count + 9].data = &pd_config_table[8];

    /* SI Cluster 2 */
    pd_config_table[9].pd_type = MOD_PD_TYPE_CLUSTER;
    pd_config_table[9].ppu.reg_base = SCP_PPU_SI_CLUS2;
    pd_config_table[9].ppu.irq = FWK_INTERRUPT_NONE;
    pd_config_table[9].observer_id = FWK_ID_NONE;
    pd_config_table[9].observer_api = FWK_ID_NONE;
    element_table[core_count + cluster_count + 10].name = "CLUS2";
    element_table[core_count + cluster_count + 10].data = &pd_config_table[9];

    /*
     * Configure pd_source_id with the SYSTOP identifier from the power domain
     * module which is dynamically defined based on the number of cores.
     */
    ppu_v1_config_data.pd_source_id = fwk_id_build_element_id(
        fwk_module_id_power_domain,
        core_count + cluster_count + PD_STATIC_DEV_IDX_SYSTOP);

    return element_table;
}

const struct fwk_module_config config_ppu_v1 = {
    .data = &ppu_v1_config_data,
    .elements = FWK_MODULE_DYNAMIC_ELEMENTS(ppu_v1_get_element_table),
};
