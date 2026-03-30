/*
 * Arm SCP/MCP Software
 * Copyright (c) 2024, Arm Limited and Contributors. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 * Description:
 *      System Coordinator unit test support.
 */

#ifndef MOD_SYSTEM_COORDINATOR_EXTRA_H
#define MOD_SYSTEM_COORDINATOR_EXTRA_H

#include <mod_timer.h>

#include <fwk_id.h>

int phase_api_stub(void);

int start_alarm_api(
    fwk_id_t alarm_id,
    unsigned int microseconds,
    enum mod_timer_alarm_type type,
    void (*callback)(uintptr_t param),
    uintptr_t param);

#endif /* MOD_SYSTEM_COORDINATOR_EXTRA_H */
