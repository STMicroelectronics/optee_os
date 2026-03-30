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
    FWK_MODULE_IDX_THERMAL_POWER_ESTIMATOR,
    FWK_MODULE_IDX_PID_CONTROLLER,
    FWK_MODULE_IDX_SENSOR,
    FWK_MODULE_IDX_FAKE,
    FWK_MODULE_IDX_COUNT,
};

static const fwk_id_t fwk_module_id_thermal_power_estimator =
    FWK_ID_MODULE_INIT(FWK_MODULE_IDX_THERMAL_POWER_ESTIMATOR);

static const fwk_id_t fwk_module_id_pid_controller =
    FWK_ID_MODULE_INIT(FWK_MODULE_IDX_PID_CONTROLLER);

static const fwk_id_t fwk_module_id_sensor =
    FWK_ID_MODULE_INIT(FWK_MODULE_IDX_SENSOR);

static const fwk_id_t fwk_module_id_fake =
    FWK_ID_MODULE_INIT(FWK_MODULE_IDX_FAKE);

#endif /* TEST_FWK_MODULE_MODULE_IDX_H */
