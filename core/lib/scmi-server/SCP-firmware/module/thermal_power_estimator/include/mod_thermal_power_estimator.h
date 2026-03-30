/*
 * Arm SCP/MCP Software
 * Copyright (c) 2024, Arm Limited and Contributors. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef MOD_THERMAL_POWER_ESTIMATOR_H
#define MOD_THERMAL_POWER_ESTIMATOR_H

#include <fwk_element.h>
#include <fwk_id.h>
#include <fwk_module_idx.h>

#include <stdint.h>

/*!
 * \ingroup GroupModules
 * \defgroup GroupThermalPowerEstimator Thermal Power Estimator
 *
 * \details Module for estimating maximum power of a platform based
 *      on thermal input.
 *
 * \{
 */

/*!
 * \brief Thermal Power Estimator device configuration.
 */
struct mod_thermal_power_estimator_dev_config {
    /*! The thermal design power (TDP) for all the devices being controlled. */
    uint32_t tdp;

    /*! Temperature sensor identifier. */
    fwk_id_t sensor_id;

    /*! Temperature sensor API identifier. */
    fwk_id_t sensor_api_id;

    /*! PID controller identifier. */
    fwk_id_t pid_controller_id;

    /*! PID controller API identifier. */
    fwk_id_t pid_controller_api_id;
};

/*!
 * \brief API indices.
 */
enum mod_thermal_power_estimator_api_idx {
    /*! Index for limit API. */
    MOD_THERMAL_POWER_ESTIMATOR_API_IDX_POWER_MANAGEMENT,

    /*! Number of defined APIs. */
    MOD_THERMAL_POWER_ESTIMATOR_API_IDX_COUNT,
};

/*!
 * \}
 */

#endif /* MOD_THERMAL_POWER_ESTIMATOR_H */
