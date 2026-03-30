/*
 * Arm SCP/MCP Software
 * Copyright (c) 2025, Arm Limited and Contributors. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef TEST_FWK_MODULE_IDX_H
#define TEST_FWK_MODULE_IDX_H

#include <fwk_id.h>

enum fwk_module_idx {
    FWK_MODULE_IDX_FMU,
    FWK_MODULE_IDX_TIMER,
    FWK_MODULE_IDX_FAKE,
    FWK_MODULE_IDX_COUNT,
};

static const fwk_id_t fwk_module_id_fmu =
    FWK_ID_MODULE_INIT(FWK_MODULE_IDX_FMU);

static const fwk_id_t fwk_module_id_fake =
    FWK_ID_MODULE_INIT(FWK_MODULE_IDX_FAKE);

#endif /* TEST_FWK_MODULE_IDX_H */
