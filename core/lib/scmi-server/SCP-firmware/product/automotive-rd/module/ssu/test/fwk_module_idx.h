/*
 * Arm SCP/MCP Software
 * Copyright (c) 2025, Arm Limited and Contributors. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef TEST_FWK_MODULE_MODULE_IDX_H
#define TEST_FWK_MODULE_MODULE_IDX_H

#include <fwk_id.h>

enum config_ssu_module_idx {
    FWK_MODULE_IDX_SSU,
    FWK_MODULE_IDX_FMU,
    FWK_MODULE_IDX_COUNT,
    FWK_MODULE_IDX_FAKE,
};

enum config_ssu_element_idx {
    CONFIG_SSU_ELEMENT_IDX,
    CONFIG_SSU_ELEMENT_IDX_COUNT,
};

static const fwk_id_t fwk_module_id_ssu =
    FWK_ID_MODULE_INIT(FWK_MODULE_IDX_SSU);

static const fwk_id_t fwk_element_id_ssu =
    FWK_ID_ELEMENT_INIT(FWK_MODULE_IDX_SSU, 0);

static const fwk_id_t fwk_module_id_fake =
    FWK_ID_MODULE_INIT(FWK_MODULE_IDX_FAKE);

#endif /* TEST_FWK_MODULE_MODULE_IDX_H */
