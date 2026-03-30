/*
 * Arm SCP/MCP Software
 * Copyright (c) 2025, Arm Limited and Contributors. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 */

#include "test_fmu_extras.h"

#include <mod_fmu.h>
#include <mod_sbistc.h>

static struct mod_fmu_api test_fmu_api = {
    .set_enabled = test_fmu_set_enabled,
    .get_count = test_fmu_get_count,
};

static struct sbistc_fault_config test_faults[2] = {
    [0] = { .fmu_device_id = 1,
            .fmu_node_id = 2,
            .flt_name = "FAULT1",
            .handler = NULL },
    [1] = { .fmu_device_id = 3,
            .fmu_node_id = 4,
            .flt_name = "FAULT2",
            .handler = NULL },
};

static struct mod_sbistc_config test_config = {
    .count = 2,
    .flt_cfgs = test_faults,
};
