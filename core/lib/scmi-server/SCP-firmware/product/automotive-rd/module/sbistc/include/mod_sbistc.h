/*
 * Arm SCP/MCP Software
 * Copyright (c) 2025, Arm Limited and Contributors. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 * Description:
 *     SBISTC (Software Built-In Self-Test Controller) module.
 */

#ifndef MOD_SBISTC_H
#define MOD_SBISTC_H

#include <stdbool.h>
#include <stdint.h>

/*!
 * \addtogroup GroupSBISTCModule SBISTC Product Modules
 * \{
 */

/*!
 * \defgroup SBISTC Driver
 *
 * \brief Driver support for SBISTC.
 *
 * \details This module provides APIs for configuring and monitoring SBISTC
 * faults, including enabling/disabling faults and registering fault handlers.
 *
 * \{
 */

/*!
 * \brief SBISTC API indices.
 */
enum mod_sbistc_api_idx {
    /*! Default SBISTC API index */
    MOD_SBISTC_API_IDX_DEFAULT,

    /*! Number of APIs */
    MOD_SBISTC_API_IDX_COUNT
};

/*!
 * \brief SBISTC API structure.
 */
struct mod_sbistc_api {
    /*!
     * \brief Get the number of SBISTC faults.
     *
     * \param fault_id Fault identifier.
     * \param[out] count Pointer to the variable that receives the fault count.
     *
     * \retval ::FWK_SUCCESS Operation succeeded.
     * \return One of the other specific error codes described by the framework.
     */
    int (*get_count)(uint8_t fault_id, uint8_t *count);

    /*!
     * \brief Enable or disable a specific SBISTC fault.
     *
     * \param fault_id Fault identifier.
     * \param enable When set to true, enables the fault.
     *
     * \retval ::FWK_SUCCESS Operation succeeded.
     * \return One of the other specific error codes described by the framework.
     */
    int (*set_enabled)(uint8_t fault_id, bool enable);

    /*!
     * \brief Set the handler function for a specific SBISTC fault.
     *
     * \param fault_id Fault identifier.
     * \param handler Pointer to the handler function.
     *
     * \retval ::FWK_SUCCESS Operation succeeded.
     * \return One of the other specific error codes described by the framework.
     */
    int (*set_handler)(uint8_t fault_id, void (*handler)(void));
};

/*!
 * \brief SBISTC fault configuration for each fault.
 */
struct sbistc_fault_config {
    /*! SBISTC fault name string */
    const char *flt_name;

    /*! FMU device ID for this SBISTC fault (always 0 for System FMU) */
    const uint16_t fmu_device_id;

    /*! FMU node ID for this SBISTC fault */
    const uint16_t fmu_node_id;

    /*! Pointer to handler function for this fault */
    void (*handler)(void);
};

/*!
 * \brief SBISTC module configuration structure.
 */
struct mod_sbistc_config {
    /*! Number of faults */
    uint32_t count;

    /*! Pointer to array of fault configs */
    struct sbistc_fault_config *flt_cfgs;
};

/*!
 * \}
 */

/*!
 * \}
 */

#endif /* MOD_SBISTC_H */
