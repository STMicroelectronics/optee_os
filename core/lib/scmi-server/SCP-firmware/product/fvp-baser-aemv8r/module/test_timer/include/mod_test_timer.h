/*
 * Arm SCP/MCP Software
 * Copyright (c) 2025, Arm Limited and Contributors. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */
#ifndef MOD_TEST_TIMER_H
#define MOD_TEST_TIMER_H

#include <fwk_id.h>

/* Configuration structure for the test-timer module */
struct mod_test_timer_config {
    fwk_id_t alarm_id;
};

#endif /* MOD_TEST_TIMER_H */
