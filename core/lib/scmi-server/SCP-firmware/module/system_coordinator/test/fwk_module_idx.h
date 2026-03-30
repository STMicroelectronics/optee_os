/*
 * Arm SCP/MCP Software
 * Copyright (c) 2024, Arm Limited and Contributors. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef TEST_FWK_MODULE_MODULE_IDX_H
#define TEST_FWK_MODULE_MODULE_IDX_H

#include <fwk_id.h>

enum fwk_module_idx {
    FWK_MODULE_IDX_SYSTEM_COORDINATOR,
    FWK_MODULE_IDX_TIMER,
    FWK_MODULE_IDX_PHASE_ONE,
    FWK_MODULE_IDX_PHASE_TWO,
    FWK_MODULE_IDX_PHASE_THREE,
    FWK_MODULE_IDX_COUNT,
};

static const fwk_id_t fwk_module_id_system_coordinator =
    FWK_ID_MODULE_INIT(FWK_MODULE_IDX_SYSTEM_COORDINATOR);

#endif /* TEST_FWK_MODULE_MODULE_IDX_H */
