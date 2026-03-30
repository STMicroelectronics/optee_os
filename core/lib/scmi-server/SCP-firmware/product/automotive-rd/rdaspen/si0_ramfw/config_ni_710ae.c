/*
 * Arm SCP/MCP Software
 * Copyright (c) 2025, Arm Limited and Contributors. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 */

#include <mod_ni_710ae.h>

#include <fwk_element.h>
#include <fwk_id.h>
#include <fwk_macros.h>
#include <fwk_module.h>
#include <fwk_module_idx.h>

enum si_ni710ae_nci_ids {
    SI_NI710AE_NCI_MHU = 0,
    SI_NI710AE_NCI_SECONDARY,
    SI_NI710AE_NCI_PRIMARY,
    SI_NI710AE_NCI_COUNT,
};

/**
 * @brief NI710AE Secondary NCI component IDs (ASNI)
 *
 * The table below lists Secondary NCI ASNI, AMNI, and PMNI components and
 * marks which are enabled in APU configuration.
 *
 * | Component ID                      | Type  | APU Enabled |
 * |-----------------------------------|-------|-------------|
 * | SI_SEC_ASNI_ICP_ICS_ID            | ASNI  | No          |
 * | SI_SEC_ASNI_SIDE_RSE_MM_S_ID      | ASNI  | Yes         |
 * -----------------------------------------------------------
 */

enum si_secondary_ASNI_ids {
    SI_SEC_ASNI_ICP_ICS_ID = 0, // Primary to Secondary Interconnect conn.
    SI_SEC_ASNI_SIDE_RSE_MM_S_ID,
};

// clang-format off
/* Secondary NCI visibilities */
static const struct ni_710ae_apu_subregion_configs
secondary_nci_asni_rse_mm[] = {
    /* Secondary Interconnect to Primary Interconnect conn. */
    /* Local SRAM Group0 */
    { 0x40000000, 0x407fffff, NCI_FOREGROUND, NCI_SEC_RW, 0, },
};
// clang-format on

static const struct ni_710ae_component_apu_config
    ni_710ae_secondary_nci_components[] = {
        {
            .component_id = SI_SEC_ASNI_SIDE_RSE_MM_S_ID,
            .component_type = NI710AE_NODE_TYPE_ASNI,
            .regions = secondary_nci_asni_rse_mm,
            .apu_subregion_count = FWK_ARRAY_SIZE(secondary_nci_asni_rse_mm),
        },
    };

/**
 * @brief NI710AE Primary NCI component IDs (ASNI & AMNI)
 *
 * The table below lists primary ASNI and AMNI components and
 * marks which are enabled in APU configuration.
 *
 * | Component ID                        | Type  | APU Enabled |
 * |-------------------------------------|-------|-------------|
 * | SI_PRIMARY_ASNI_CLUSTER0_MM_ID      | ASNI  | Yes         |
 * | SI_PRIMARY_ASNI_CLUSTER0_SPP_ID     | ASNI  | No          |
 * | SI_PRIMARY_ASNI_DTE_ID              | ASNI  | No          |
 * | SI_PRIMARY_ASNI_EXP_SUB_ID          | ASNI  | Yes         |
 * | SI_PRIMARY_ASNI_ICS_ICP_ACC_ID      | ASNI  | Yes         |
 * | SI_PRIMARY_ASNI_IO_EXTENSION_ID     | ASNI  | Yes         |
 * |-------------------------------------|-------|-------------|
 * | SI_PRIMARY_AMNI_SH1_ACC_ID          | AMNI  | Yes         |
 * | SI_PRIMARY_AMNI_GIC_ID              | AMNI  | No          |
 * | SI_PRIMARY_AMNI_ROM_ACC_ID          | AMNI  | No          |
 * | SI_PRIMARY_AMNI_ICM_CL0_ID          | AMNI  | No          |
 * | SI_PRIMARY_AMNI_ICM_DTE_ID          | AMNI  | No          |
 * | SI_PRIMARY_AMNI_ATU_ACC_MA_ID       | AMNI  | No          |
 * | SI_PRIMARY_AMNI_ATU_ACC_PA_ID       | AMNI  | No          |
 * | SI_PRIMARY_AMNI_CLUSTER0_ACEL_ID    | AMNI  | No          |
 * | SI_PRIMARY_AMNI_CLUSTER0_UTILITY_ID | AMNI  | No          |
 * | SI_PRIMARY_AMNI_EXP0_ID             | AMNI  | No          |
 * | SI_PRIMARY_AMNI_ICS_CFG_ID          | AMNI  | No          |
 * | SI_PRIMARY_AMNI_QSPI_ID             | AMNI  | No          |
 * -------------------------------------------------------------
 */

enum si_primary_ASNI_ids {
    SI_PRIMARY_ASNI_CLUSTER0_MM_ID = 0,
    SI_PRIMARY_ASNI_CLUSTER0_SPP_ID,
    SI_PRIMARY_ASNI_DTE_ID, // Debug Trace Extension
    SI_PRIMARY_ASNI_EXP_SUB_ID,
    SI_PRIMARY_ASNI_ICS_ICP_ACC_ID, // Secondary to Primary Interconnect conn.
    SI_PRIMARY_ASNI_IO_EXTENSION_ID,
};

enum si_primary_AMNI_ids {
    SI_PRIMARY_AMNI_EXP0_ID = 0,
    SI_PRIMARY_AMNI_ATU_ACC_MA_ID,
    SI_PRIMARY_AMNI_ATU_ACC_PA_ID,
    SI_PRIMARY_AMNI_CLUSTER0_ACEL_ID,
    SI_PRIMARY_AMNI_CLUSTER0_UTILITY_ID,
    SI_PRIMARY_AMNI_GIC_ID,
    SI_PRIMARY_AMNI_ICM_CL0_ID, // MHU Interconnect CL0
    SI_PRIMARY_AMNI_ICM_DTE_ID, // MHU Interconnect DTE
    SI_PRIMARY_AMNI_ICS_CFG_ID, // Secondary Interconnect Configuration
    SI_PRIMARY_AMNI_ROM_ACC_ID, // Boot ROM
    SI_PRIMARY_AMNI_SH1_ACC_ID, // SRAM0
    SI_PRIMARY_AMNI_QSPI_ID,
};

// clang-format off
/* Primary NCI visibilities */
static const struct ni_710ae_apu_subregion_configs primary_nci_asni_cluster0_mm[] =
{
    /* Broad Visibility */
    { 0x0000000000, 0xffffffffff, NCI_BACKGROUND, NCI_SEC_RW, 0, },
    /* Base peripherals Prog - system_control_register */
    { 0x2a6b0000, 0x2a6bffff, NCI_FOREGROUND, NCI_SEC_R, 1, },
};

static const struct ni_710ae_apu_subregion_configs
primary_nci_asni_expansion_port0[] = {
    /* QSPI */
    { 0x60000000, 0x7fffffff, NCI_FOREGROUND, NCI_SEC_RW, 0, },
    /* GPIO */
    { 0x8800000, 0x880ffff, NCI_FOREGROUND, NCI_SEC_RW, 1 },
    /* ATU PCMA */
    { 0x80000000, 0xffffffff, NCI_FOREGROUND, NCI_SEC_RW, 2, },
    /* ATU PCPA */
    { 0x18000000, 0x1fffffff, NCI_FOREGROUND, NCI_SEC_RW, 3, },
    /* Local SRAM Group-0 */
    { 0x40000000, 0x407fffff, NCI_FOREGROUND, NCI_SEC_RW, 4, },
};

/* Secondary to Primary Interconnect interface */
static const struct ni_710ae_apu_subregion_configs primary_nci_asni_ics_icp_acc[] =
{
    /* Local SRAM Group-0 */
    { 0x40000000, 0x407fffff, NCI_FOREGROUND, NCI_SEC_RW, 0, },
};

static const struct ni_710ae_apu_subregion_configs primary_nci_asni_ioe[] = {
    /* Expansion Port0 */
    { 0x600000000, 0x63fffffff, NCI_FOREGROUND, NCI_SEC_RW, 0, },
    /* CL0 Acels TCM */
    { 0x110000000, 0x110ffffff, NCI_FOREGROUND, NCI_SEC_RW, 1, },
    /* CL0 Acels LLRAM */
    { 0x120000000, 0x12fffffff, NCI_FOREGROUND, NCI_SEC_RW, 2, },
    /* Local SRAM Group-0 */
    { 0x40000000, 0x407fffff, NCI_FOREGROUND, NCI_SEC_RW, 3, },
    /* ATU PCMA */
    { 0x80000000, 0xffffffff, NCI_FOREGROUND, NCI_SEC_RW, 4, },
};

static const struct ni_710ae_apu_subregion_configs primary_nci_amni_sh1_acc[] =
{
    /* Local SRAM Group-0 */
    { 0x40000000, 0x407fffff, NCI_FOREGROUND, NCI_SEC_RW, 0, },
};
// clang-format on

static const struct ni_710ae_component_apu_config
    ni_710ae_primary_nci_components[] = {
        /* ASNI – Cluster0 MM */
        {
            .component_id = SI_PRIMARY_ASNI_CLUSTER0_MM_ID,
            .component_type = NI710AE_NODE_TYPE_ASNI,
            .regions = (struct ni_710ae_apu_subregion_configs *)
                primary_nci_asni_cluster0_mm,
            .apu_subregion_count = FWK_ARRAY_SIZE(primary_nci_asni_cluster0_mm),
        },
        /* ASNI – Expansion Sub. Port0 */
        {
            .component_id = SI_PRIMARY_ASNI_EXP_SUB_ID,
            .component_type = NI710AE_NODE_TYPE_ASNI,
            .regions = (struct ni_710ae_apu_subregion_configs *)
                primary_nci_asni_expansion_port0,
            .apu_subregion_count =
                FWK_ARRAY_SIZE(primary_nci_asni_expansion_port0),
        },
        /* ASNI – Secondary to Primary Interconnect Node */
        {
            .component_id = SI_PRIMARY_ASNI_ICS_ICP_ACC_ID,
            .component_type = NI710AE_NODE_TYPE_ASNI,
            .regions = (struct ni_710ae_apu_subregion_configs *)
                primary_nci_asni_ics_icp_acc,
            .apu_subregion_count = FWK_ARRAY_SIZE(primary_nci_asni_ics_icp_acc),
        },
        /* ASNI – IO Extension */
        {
            .component_id = SI_PRIMARY_ASNI_IO_EXTENSION_ID,
            .component_type = NI710AE_NODE_TYPE_ASNI,
            .regions =
                (struct ni_710ae_apu_subregion_configs *)primary_nci_asni_ioe,
            .apu_subregion_count = FWK_ARRAY_SIZE(primary_nci_asni_ioe),
        },
        /* AMNI – Local SRAM Group-0 */
        {
            .component_id = SI_PRIMARY_AMNI_SH1_ACC_ID,
            .component_type = NI710AE_NODE_TYPE_AMNI,
            .regions = (struct ni_710ae_apu_subregion_configs *)
                primary_nci_amni_sh1_acc,
            .apu_subregion_count = FWK_ARRAY_SIZE(primary_nci_amni_sh1_acc),
        },
    };

/**
 * @brief NI710AE MHU NCI component IDs (ASNI)
 *
 * The table below lists MHU ASNI components and marks which are enabled
 * in APU configuration.
 *
 * | Component ID                      | Type  | APU Enabled |
 * |-----------------------------------|-------|-------------|
 * | SI_MHU_ASNI_CL0_MHU_PROG_ID       | ASNI  | No          |
 * | SI_MHU_ASNI_DTE_MHU_PROG_ID       | ASNI  | No          |
 * | SI_MHU_ASNI_PC_MHU_PROG_ID        | ASNI  | No          |
 * | SI_MHU_ASNI_RSE_MHU_PROG_ID       | ASNI  | Yes         |
 * -----------------------------------------------------------
 */

enum si_mhu_asni_ids {
    SI_MHU_ASNI_CL0_MHU_PROG_ID = 0, // Primary to MHU interconnect conn.
    SI_MHU_ASNI_DTE_MHU_PROG_ID,
    SI_MHU_ASNI_PC_MHU_PROG_ID,
    SI_MHU_ASNI_RSE_MHU_PROG_ID,
};

// clang-format off
/* MHU NCI visibilities */
static const struct ni_710ae_apu_subregion_configs
mhu_nci_asni_rse_mhu_prog[] = {
    /* RSE MHU Space */
    { 0x3c000000, 0x3c0bffff, NCI_FOREGROUND, NCI_SEC_RW, 0, },
};
// clang-format on

static const struct ni_710ae_component_apu_config
    ni_710ae_mhu_nci_components[] = {
        {
            .component_id = SI_MHU_ASNI_RSE_MHU_PROG_ID,
            .component_type = NI710AE_NODE_TYPE_ASNI,
            .regions = mhu_nci_asni_rse_mhu_prog,
            .apu_subregion_count = FWK_ARRAY_SIZE(mhu_nci_asni_rse_mhu_prog),
        },

    };

/* List of static APU Config elements */
static struct fwk_element ni_710ae_element_table[] = {
    [SI_NI710AE_NCI_MHU] = {
        .name = "SI_MHU_NCI",
        .data = &((struct mod_ni_710ae_element_config){
            .periphbase_addr = 0x00002A300000ULL,
            .apu_configs = ni_710ae_mhu_nci_components,
            .apu_config_count = FWK_ARRAY_SIZE(ni_710ae_mhu_nci_components),
            .max_number_of_nodes = 32,
        }),
    },
    [SI_NI710AE_NCI_SECONDARY] = {
        .name = "SI_SECONDARY_NCI",
        .data = &((struct mod_ni_710ae_element_config){
            .periphbase_addr = 0x00002A200000ULL,
            .apu_configs = ni_710ae_secondary_nci_components,
            .apu_config_count = FWK_ARRAY_SIZE(ni_710ae_secondary_nci_components),
            .max_number_of_nodes = 32,
        }),
    },
    [SI_NI710AE_NCI_PRIMARY] = {
        .name = "SI_PRIMARY_NCI",
        .data = &((struct mod_ni_710ae_element_config){
            .periphbase_addr = 0x00002A000000ULL,
            .apu_configs = ni_710ae_primary_nci_components,
            .apu_config_count = FWK_ARRAY_SIZE(ni_710ae_primary_nci_components),
            .max_number_of_nodes = 64,
        }),
    },
    [SI_NI710AE_NCI_COUNT] = { 0 },
};

const struct fwk_module_config config_ni_710ae = {
    .elements = FWK_MODULE_STATIC_ELEMENTS_PTR(ni_710ae_element_table),
};
