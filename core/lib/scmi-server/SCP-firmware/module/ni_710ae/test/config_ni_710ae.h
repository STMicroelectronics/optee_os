/*
 * Arm SCP/MCP Software
 * Copyright (c) 2025, Arm Limited and Contributors. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <mod_ni_710ae.h>

#include <fwk_id.h>
#include <fwk_macros.h>
#include <fwk_module.h>
#include <fwk_module_idx.h>

#define MAX_NODE_COUNT_FOR_DISCOVERY_DUMMY_NCI_PRIMARY   (32U)
#define MAX_NODE_COUNT_FOR_DISCOVERY_DUMMY_NCI_SECONDARY (16U)

enum dummy_ni_710ae_nci_primary_ids {
    DUMMY_NI_710AE_NCI_PRIMARY = 0,
    DUMMY_NI_710AE_NCI_SECONDARY,

    DUMMY_NI_710AE_NCI_COUNT,
};

/* DUMMY PRIMARY NI_710AE*/
/* Interface IDs NCI ASNI components */
enum dummy_ni_710ae_primary_ASNI_ids {
    DUMMY_NI_710AE_PRIMARY_ASNI_ID_0 = 0,
    DUMMY_NI_710AE_PRIMARY_ASNI_ID_1,
};
/* Interface IDs NCI AMNI components */
enum dummy_ni_710ae_primary_AMNI_ids {
    DUMMY_NI_710AE_PRIMARY_AMNI_ID_0 = 0,
    DUMMY_NI_710AE_PRIMARY_AMNI_ID_1,
};

/* DUMMY SECONDARY NI_710AE*/
/* Interface IDs NCI ASNI components */
enum dummy_ni_710ae_secondary_ASNI_ids {
    DUMMY_NI_710AE_SECONDARY_ASNI_ID_0 = 0,
};

// clang-format off
static const struct ni_710ae_apu_subregion_configs
dummy_nci_primary_asni_id_0[2] = {
    { 0xDEDEDEDE, 0xDEFDEFDE, NCI_FOREGROUND, NCI_SEC_RW, 0 },
    { 0xBEBEBEBE, 0xCECECECE, NCI_FOREGROUND, NCI_SEC_RW, 1 },
};
static const struct ni_710ae_apu_subregion_configs
dummy_nci_primary_asni_id_1[2] = {
    { 0xDEDEDEDE, 0xDEFDEFDE, NCI_FOREGROUND, NCI_SEC_RW, 0 },
    { 0xBEBEBEBE, 0xCECECECE, NCI_FOREGROUND, NCI_SEC_RW, 1 },
};
static const struct ni_710ae_apu_subregion_configs
dummy_nci_primary_amni_id_0[2] = {
    { 0xDEDEDEDE, 0xDEFDEFDE, NCI_FOREGROUND, NCI_SEC_RW, 0 },
    { 0xBEBEBEBE, 0xCECECECE, NCI_FOREGROUND, NCI_SEC_RW, 1 },
};
// clang-format on

// clang-format off
static const struct ni_710ae_apu_subregion_configs
dummy_nci_secondary_asni_id_0[2] = {
    { 0xDEDEDEDE, 0xDEFDEFDE, NCI_FOREGROUND, NCI_SEC_RW, 0 },
    { 0xBEBEBEBE, 0xCECECECE, NCI_FOREGROUND, NCI_SEC_RW, 1 },
};
// clang-format on

static const struct ni_710ae_component_apu_config
    ni_710ae_dummy_nci_primary_components[3] = {
        {
            .component_id = DUMMY_NI_710AE_PRIMARY_ASNI_ID_0,
            .component_type = NI710AE_NODE_TYPE_ASNI,
            .regions = dummy_nci_primary_asni_id_0,
            .apu_subregion_count = sizeof(dummy_nci_primary_asni_id_0) /
                sizeof(dummy_nci_primary_asni_id_0[0]),
        },
        {
            .component_id = DUMMY_NI_710AE_PRIMARY_ASNI_ID_1,
            .component_type = NI710AE_NODE_TYPE_ASNI,
            .regions = dummy_nci_primary_asni_id_1,
            .apu_subregion_count = sizeof(dummy_nci_primary_asni_id_1) /
                sizeof(dummy_nci_primary_asni_id_1[0]),
        },
        {
            .component_id = DUMMY_NI_710AE_PRIMARY_AMNI_ID_0,
            .component_type = NI710AE_NODE_TYPE_AMNI,
            .regions = dummy_nci_primary_amni_id_0,
            .apu_subregion_count = sizeof(dummy_nci_primary_amni_id_0) /
                sizeof(dummy_nci_primary_amni_id_0[0]),
        },
    };

static const struct ni_710ae_component_apu_config
    ni_710ae_dummy_nci_secondary_components[1] = {
        {
            .component_id = DUMMY_NI_710AE_SECONDARY_ASNI_ID_0,
            .component_type = NI710AE_NODE_TYPE_ASNI,
            .regions = dummy_nci_secondary_asni_id_0,
            .apu_subregion_count = sizeof(dummy_nci_secondary_asni_id_0) /
                sizeof(dummy_nci_secondary_asni_id_0[0]),
        },
    };

/* List of static APU Config elements */
static struct fwk_element ni_710ae_element_table[] = {

    [DUMMY_NI_710AE_NCI_PRIMARY] = {
        .name = "DUMMY_NCI_PRIMARY",
        .data = &((struct mod_ni_710ae_element_config){
            .periphbase_addr = 0x12345678ABCDULL,
            .apu_configs = ni_710ae_dummy_nci_primary_components,
            .apu_config_count = sizeof(ni_710ae_dummy_nci_primary_components) /
                sizeof(ni_710ae_dummy_nci_primary_components[0]),
            .max_number_of_nodes = 16,
        }),
    },
    [DUMMY_NI_710AE_NCI_SECONDARY] = {
        .name = "DUMMY_NCI_SECONDARY",
        .data = &((struct mod_ni_710ae_element_config){
            .periphbase_addr = 0x12345678ABCDULL,
            .apu_configs = ni_710ae_dummy_nci_secondary_components,
            .apu_config_count = sizeof(ni_710ae_dummy_nci_secondary_components) /
                sizeof(ni_710ae_dummy_nci_secondary_components[0]),
            .max_number_of_nodes = 32,
        }),
    },
    [DUMMY_NI_710AE_NCI_COUNT] = { 0 },
};

const struct fwk_module_config config_ni_710ae = {
    .elements = FWK_MODULE_STATIC_ELEMENTS_PTR(ni_710ae_element_table),
};

/* Register module configs */
const struct fwk_module_config
    *fwk_module_config_table[FWK_MODULE_IDX_COUNT] = {
        [FWK_MODULE_IDX_NI_710AE] = &config_ni_710ae,
        [FWK_MODULE_IDX_FAKE] = NULL,
    };
