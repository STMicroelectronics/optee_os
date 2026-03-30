/*
 * Arm SCP/MCP Software
 * Copyright (c) 2024, Arm Limited and Contributors. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 * Description:
 *      Thermal Power Estimator unit test support.
 */

#include <mod_pid_controller.h>
#include <mod_sensor.h>

/*! PID Controller update */
int mod_pid_controller_update(fwk_id_t id, int64_t input, int64_t *output);

/*! Read sensor data. */
int mod_sensor_get_data(fwk_id_t id, struct mod_sensor_data *data);
