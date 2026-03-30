/*
 * Arm SCP/MCP Software
 * Copyright (c) 2023-2024, Arm Limited and Contributors. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <tc4_core.h>
#include <tc4_mock_psu.h>
#include <tc4_psu.h>

#include <mod_mock_psu.h>
#include <mod_psu.h>

#include <fwk_element.h>
#include <fwk_module.h>
#include <fwk_module_idx.h>

static const struct fwk_element element_table[PSU_ELEMENT_IDX_COUNT + 1] = {
    [PSU_ELEMENT_IDX_GROUP_LITTLE] = {
        .name = "PSU_GROUP_" TC4_GROUP_LITTLE_NAME,
        .data =
            &(const struct mod_psu_element_cfg){
                .driver_id = FWK_ID_ELEMENT_INIT(FWK_MODULE_IDX_MOCK_PSU,
                    MOCK_PSU_ELEMENT_IDX_GROUP_LITTLE),
                .driver_api_id = FWK_ID_API_INIT(
                    FWK_MODULE_IDX_MOCK_PSU,
                    MOD_MOCK_PSU_API_IDX_DRIVER) },
    },
    [PSU_ELEMENT_IDX_GROUP_MID] = {
        .name = "PSU_GROUP_" TC4_GROUP_MID_NAME,
        .data =
            &(const struct mod_psu_element_cfg){
                .driver_id = FWK_ID_ELEMENT_INIT(FWK_MODULE_IDX_MOCK_PSU,
                    MOCK_PSU_ELEMENT_IDX_GROUP_MID),
                .driver_api_id = FWK_ID_API_INIT(
                    FWK_MODULE_IDX_MOCK_PSU,
                    MOD_MOCK_PSU_API_IDX_DRIVER) },
    },
    [PSU_ELEMENT_IDX_GROUP_BIG] = {
        .name = "PSU_GROUP_" TC4_GROUP_BIG_NAME,
        .data =
            &(const struct mod_psu_element_cfg){
                .driver_id = FWK_ID_ELEMENT_INIT(FWK_MODULE_IDX_MOCK_PSU,
                    MOCK_PSU_ELEMENT_IDX_GROUP_BIG),
                .driver_api_id = FWK_ID_API_INIT(
                    FWK_MODULE_IDX_MOCK_PSU,
                    MOD_MOCK_PSU_API_IDX_DRIVER) },
    },
    [PSU_ELEMENT_IDX_GPU] = {
        .name = "PSU_GROUP_GPU",
        .data =
            &(const struct mod_psu_element_cfg){
                .driver_id = FWK_ID_ELEMENT_INIT(FWK_MODULE_IDX_MOCK_PSU,
                    MOCK_PSU_ELEMENT_IDX_GPU),
                .driver_api_id = FWK_ID_API_INIT(
                    FWK_MODULE_IDX_MOCK_PSU,
                    MOD_MOCK_PSU_API_IDX_DRIVER) },
    },
    { 0 }
};

static const struct fwk_element *psu_get_element_table(fwk_id_t module_id)
{
    return element_table;
}

const struct fwk_module_config config_psu = {
    .elements = FWK_MODULE_DYNAMIC_ELEMENTS(psu_get_element_table),
};
