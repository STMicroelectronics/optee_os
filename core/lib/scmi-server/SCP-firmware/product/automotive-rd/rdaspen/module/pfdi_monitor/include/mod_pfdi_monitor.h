/*
 * Arm SCP/MCP Software
 * Copyright (c) 2025, Arm Limited and Contributors. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 * Description:
 *     PFDI Monitor
 */

#ifndef MOD_PFDI_MONITOR_H
#define MOD_PFDI_MONITOR_H

#include <fwk_id.h>

#include <stdint.h>

/*!
 * \brief API indices
 */
enum mod_pfdi_monitor_api_idx {
    MOD_PFDI_MONITOR_API_IDX_PFDI_MONITOR,
    MOD_PFDI_MONITOR_API_IDX_COUNT
};

/*!
 * \brief PFDI monitor API.
 */
struct mod_pfdi_monitor_api {
    /*!
     * \brief Report the Out-of-Reset PFDI status for a core
     *
     * \param id PFDI monitor core element ID
     * \param status PFDI tests status
     *
     * \retval ::FWK_SUCCESS The operation succeeded.
     * \retval ::FWK_E_PARAM One or more parameters were incorrect.
     * \retval ::FWK_E_INIT The core framework component is not initialized.
     * \retval ::FWK_E_OS Operating system error.
     *
     * \return One of the standard framework error codes.
     */
    int (*oor_status)(fwk_id_t id, uint32_t status);
    /*!
     * \brief Report the Online PFDI status for a core
     *
     * \param id PFDI monitor core element ID
     * \param status PFDI tests status
     *
     * \retval ::FWK_SUCCESS The operation succeeded.
     * \retval ::FWK_E_PARAM One or more parameters were incorrect.
     * \retval ::FWK_E_INIT The core framework component is not initialized.
     * \retval ::FWK_E_OS Operating system error.
     *
     * \return One of the standard framework error codes.
     */
    int (*onl_status)(fwk_id_t id, uint32_t status);
};

/*!
 * \brief PFDI monitor core configuration data.
 */
struct mod_pfdi_monitor_core_config {
    /*! Alarm identifier */
    fwk_id_t alarm_id;
    /*! The out-of-reset PFDI alarm interval in microseconds */
    unsigned int oor_pfdi_period_us;
    /*! The timeout for the first online PFDI in microseconds */
    unsigned int boot_timeout_us;
    /*! The online PFDI alarm interval in microseconds */
    unsigned int onl_pfdi_period_us;
};

#endif /* MOD_PFDI_MONITOR_H */
