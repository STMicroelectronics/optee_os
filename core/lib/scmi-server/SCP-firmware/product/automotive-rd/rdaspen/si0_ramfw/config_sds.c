/*
 * Arm SCP/MCP Software
 * Copyright (c) 2025, Arm Limited and Contributors. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 * Description:
 *     Configuration data for module 'sds'.
 */

#include "si0_cfgd_sds.h"
#include "si0_mmap.h"

#include <mod_sds.h>
#include <mod_si0_platform.h>

#include <fwk_assert.h>
#include <fwk_element.h>
#include <fwk_id.h>
#include <fwk_macros.h>
#include <fwk_module.h>
#include <fwk_module_idx.h>

#include <stdint.h>

#define MOD_SDS_ELEMENT_COUNT (SI0_CFGD_MOD_SDS_EIDX_COUNT + 1)
#define SDS_REGION_COUNT      SI0_CFGD_MOD_SDS_REGION_IDX_COUNT

static const uint32_t version_packed = FWK_BUILD_VERSION;
static const uint32_t feature_flags;

static const struct mod_sds_region_desc sds_regions[SDS_REGION_COUNT] = {
    [SI0_CFGD_MOD_SDS_REGION_IDX_SECURE] = {
        .base = (void*)SI0_SDS_SECURE_BASE,
        .size = SI0_SDS_SECURE_SIZE,
    },
};

static_assert(
    FWK_ARRAY_SIZE(sds_regions) == SI0_CFGD_MOD_SDS_REGION_IDX_COUNT,
    "Mismatch between number of SDS regions and number of regions "
    "provided by the SDS configuration.");

const struct mod_sds_config sds_module_config = {
    .regions = sds_regions,
    .region_count = SI0_CFGD_MOD_SDS_REGION_IDX_COUNT,
    .clock_id = FWK_ID_NONE_INIT,
    .platform_notification = {
        .notification_id = FWK_ID_NOTIFICATION_INIT(
            FWK_MODULE_IDX_SI0_PLATFORM,
            MOD_SI0_PLATFORM_NOTIFICATION_IDX_SUBSYS_INITIALIZED),
        .source_id = FWK_ID_MODULE_INIT(
            FWK_MODULE_IDX_SI0_PLATFORM),
    },
};

static struct fwk_element sds_element_table[MOD_SDS_ELEMENT_COUNT] = {
    [SI0_CFGD_MOD_SDS_EIDX_CPU_INFO] = {
        .name = "CPU Info",
        .data = &((struct mod_sds_structure_desc){
            .id = SDS_AP_CPU_INFO_STRUCT_ID,
            .size = SI0_CFGD_MOD_SDS_CPU_INFO_SIZE,
            .region_id = SI0_CFGD_MOD_SDS_REGION_IDX_SECURE,
            .finalize = true,
        }),
    },
    [SI0_CFGD_MOD_SDS_EIDX_ROM_VERSION] = {
        .name = "ROM firmware version",
        .data = &((struct mod_sds_structure_desc){
            .id = SDS_ROM_VERSION_STRUCT_ID,
            .size = SI0_CFGD_MOD_SDS_ROM_VERSION_SIZE,
            .payload = &version_packed,
            .region_id = SI0_CFGD_MOD_SDS_REGION_IDX_SECURE,
            .finalize = true,
        }),
    },
    [SI0_CFGD_MOD_SDS_EIDX_RAM_VERSION] = {
        .name = "RAM firmware version",
        .data = &((struct mod_sds_structure_desc){
            .id = SDS_RAM_VERSION_STRUCT_ID,
            .size = SI0_CFGD_MOD_SDS_RAM_VERSION_SIZE,
            .payload = &version_packed,
            .region_id = SI0_CFGD_MOD_SDS_REGION_IDX_SECURE,
            .finalize = true,
        }),
    },
    [SI0_CFGD_MOD_SDS_EIDX_RESET_SYNDROME] = {
        .name = "Reset Syndrome",
        .data = &((struct mod_sds_structure_desc){
            .id = SDS_RESET_SYNDROME_STRUCT_ID,
            .size = SI0_CFGD_MOD_SDS_RESET_SYNDROME_SIZE,
            .region_id = SI0_CFGD_MOD_SDS_REGION_IDX_SECURE,
            .finalize = true,
        }),
    },
    [SI0_CFGD_MOD_SDS_EIDX_FEATURE_AVAILABILITY] = {
        .name = "Feature Availability",
        .data = &((struct mod_sds_structure_desc){
            .id = SDS_FEATURE_AVAIL_STRUCT_ID,
            .size = SI0_CFGD_MOD_SDS_FEATURE_AVAILABILITY_SIZE,
            .payload = &feature_flags,
            .region_id = SI0_CFGD_MOD_SDS_REGION_IDX_SECURE,
            .finalize = true,
        }),
    },
    [SI0_CFGD_MOD_SDS_ISOLATED_CPU_MPID] = {
        .name = "Isolated CPU MPID List",
        .data = &((struct mod_sds_structure_desc){
                .id = SDS_ISOLATED_CPU_MPID_STRUCT_ID,
                .size = SI0_CFGD_MOD_SDS_ISOLATED_CPU_MPID_SIZE,
                .region_id = SI0_CFGD_MOD_SDS_REGION_IDX_SECURE,
                .finalize = true,
                }),
    },
    [SI0_CFGD_MOD_SDS_EIDX_COUNT] = { 0 }, /* Termination description. */
};

static_assert(
    SI0_SDS_SECURE_SIZE > SI0_CFGD_MOD_SDS_CPU_INFO_SIZE +
            SI0_CFGD_MOD_SDS_ROM_VERSION_SIZE +
            SI0_CFGD_MOD_SDS_RAM_VERSION_SIZE +
            SI0_CFGD_MOD_SDS_RESET_SYNDROME_SIZE +
            SI0_CFGD_MOD_SDS_FEATURE_AVAILABILITY_SIZE +
            SI0_CFGD_MOD_SDS_ISOLATED_CPU_MPID_SIZE,
    "SDS structures too large for SDS SRAM.\n");

static const struct fwk_element *sds_get_element_table(fwk_id_t module_id)
{
    static_assert(BUILD_VERSION_MAJOR < UINT8_MAX, "Invalid version size");
    static_assert(BUILD_VERSION_MINOR < UINT8_MAX, "Invalid version size");
    static_assert(BUILD_VERSION_PATCH < UINT16_MAX, "Invalid version size");

    return sds_element_table;
}

struct fwk_module_config config_sds = {
    .data = &sds_module_config,
    .elements = FWK_MODULE_DYNAMIC_ELEMENTS(sds_get_element_table),
};
