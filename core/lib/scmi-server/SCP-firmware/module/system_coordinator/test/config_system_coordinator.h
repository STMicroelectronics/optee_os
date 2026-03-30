/*
 * Arm SCP/MCP Software
 * Copyright (c) 2024, Arm Limited and Contributors. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <mod_system_coordinator.h>

#include <fwk_id.h>
#include <fwk_module.h>
#include <fwk_module_idx.h>

enum phase_idx {
    SYS_COOR_PHASE_ONE,
    SYS_COOR_PHASE_TWO,
    SYS_COOR_PHASE_THREE,
    SYS_COOR_PHASE_COUNT,
};

#define PHASE_ONE_TIME         2000
#define PHASE_TWO_TIME         0
#define PHASE_THREE_TIME       3000
#define COORDINATOR_CYCLE_TIME 10000

static const struct fwk_element phase_config[] = {
    [SYS_COOR_PHASE_ONE] = {
        .name = "PHASE_ONE",
        .data = &(const struct mod_system_coordinator_phase_config) {
            .module_id = FWK_ID_MODULE_INIT(FWK_MODULE_IDX_PHASE_ONE),
            .api_id = FWK_ID_API_INIT(FWK_MODULE_IDX_PHASE_ONE, 0),
            .phase_us = PHASE_ONE_TIME,
        },
    },
    [SYS_COOR_PHASE_TWO] = {
        .name = "PHASE_TWO",
        .data = &(const struct mod_system_coordinator_phase_config) {
            .module_id = FWK_ID_MODULE_INIT(FWK_MODULE_IDX_PHASE_TWO),
            .api_id = FWK_ID_API_INIT(FWK_MODULE_IDX_PHASE_TWO, 0),
            .phase_us = PHASE_TWO_TIME,
        },
    },
    [SYS_COOR_PHASE_THREE] = {
        .name = "PHASE_THREE",
        .data = &(const struct mod_system_coordinator_phase_config) {
            .module_id = FWK_ID_MODULE_INIT(FWK_MODULE_IDX_PHASE_THREE),
            .api_id = FWK_ID_API_INIT(FWK_MODULE_IDX_PHASE_THREE, 0),
            .phase_us = PHASE_THREE_TIME,
        },
    },
    [SYS_COOR_PHASE_COUNT] = { 0 },
};

struct mod_system_coordinator_config system_coordinator_config = {
    .cycle_alarm_id = FWK_ID_SUB_ELEMENT_INIT(FWK_MODULE_IDX_TIMER, 0, 0),
    .phase_alarm_id = FWK_ID_SUB_ELEMENT_INIT(FWK_MODULE_IDX_TIMER, 0, 0),
    .cycle_us = COORDINATOR_CYCLE_TIME
};

struct fwk_module_config module_config = {
    .data = &system_coordinator_config,
    .elements = FWK_MODULE_STATIC_ELEMENTS_PTR(phase_config),
};
