/*
 * Arm SCP/MCP Software
 * Copyright (c) 2024, Arm Limited and Contributors. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef MOD_SYSTEM_COORDINATOR_H
#define MOD_SYSTEM_COORDINATOR_H

#include <fwk_id.h>

/*!
 * \addtogroup GroupModules Modules
 * \{
 */

/*!
 * \defgroup GroupSystemCoordinator System Coordinator
 * \{
 */

/*!
 * \brief System Coordinator Phase API
 *
 * \details API interface for System Coordinator to call each phase.
 */
struct mod_system_coordinator_phase_api {
    /*!
     * \brief API interface to phase module
     *
     * \retval ::FWK_SUCCESS The operation succeeded.
     * \retval ::Standard framework status codes.
     */
    int (*phase_handler)(void);
};

/*!
 * \brief System Coordinator phase config
 */
struct mod_system_coordinator_phase_config {
    /*!
     * ID of the phase module.
     */
    const fwk_id_t module_id;

    /*!
     * ID of the phase API.
     */
    const fwk_id_t api_id;

    /*!
     * \brief Phase time in microseconds
     */
    uint32_t phase_us;
};

/*!
 * \brief System Coordinator module configuration
 */
struct mod_system_coordinator_config {
    /*!
     * \brief Identifier of alarm for System Coordinator cycle
     */
    fwk_id_t cycle_alarm_id;

    /*!
     * \brief Identifier of alarm for System Coordinator phase
     */
    fwk_id_t phase_alarm_id;

    /*!
     * \brief Cycle time in microseconds
     */
    uint32_t cycle_us;
};

/*!
 * \}
 */

/*!
 * \}
 */

#endif /* MOD_SYSTEM_COORDINATOR_H */
