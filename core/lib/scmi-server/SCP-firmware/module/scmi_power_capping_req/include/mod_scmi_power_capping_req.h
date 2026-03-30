/*
 * Arm SCP/MCP Software
 * Copyright (c) 2025, Arm Limited and Contributors. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef MOD_SCMI_POWER_CAPPING_REQ_H
#define MOD_SCMI_POWER_CAPPING_REQ_H

#include <fwk_id.h>
#include <fwk_macros.h>
#include <fwk_module_idx.h>

#include <stdint.h>

/*!
 * \brief API indices
 */
enum mod_power_capping_req_api_idx {
    /*! API used for sending SCMI commands and receive responses */
    MOD_POW_CAP_REQ_API_IDX_SCMI_REQ,

    /*! API used to set the cap from another module */
    MOD_POW_CAP_REQ_API_IDX_REQ,

    /*! Power Management API. */
    MOD_POW_CAP_REQ_API_IDX_LIMITER_POWER_API,

    /*! Number of defined APIs */
    MOD_POW_CAP_REQ_API_IDX_COUNT,
};

/*!
 * \brief SCMI system power platform configuration
 */
struct mod_scmi_power_capping_req_dev_config {
    /*!
     * \brief SCMI Service ID
     *
     * \details The service ID which corresponds to the required
     *      channel in the transport layer.
     */
    fwk_id_t service_id;

    /*!
     * \brief Identifier of the alarm for response timeout.
     */
    fwk_optional_id_t alarm_id;

    /*!
     * Alarm delay period for the timeout.
     */
    uint32_t alarm_delay;
};

/*!
 * \brief APIs provided by the Power Capping Requester module
 */
struct mod_scmi_power_capping_req_api {
    /*!
     * \brief Set power cap for a domain
     *
     * \param domain_id Identifier of the power domain
     * \param power_cap Desired power cap value
     * \param flags Flags for the set power cap command
     *
     * \retval FWK_SUCCESS Operation succeeded.
     * \retval FWK_E_PARAM Invalid parameters provided.
     * \return One of the standard framework error codes.
     */
    int (
        *set_power_cap)(fwk_id_t domain_id, uint32_t power_cap, uint32_t flags);
};

/*! Identifier of the power capping req API */
static const fwk_id_t mod_power_capping_req_api_id = FWK_ID_API_INIT(
    FWK_MODULE_IDX_SCMI_POWER_CAPPING_REQ,
    MOD_POW_CAP_REQ_API_IDX_REQ);

/*! Identifier of the PCAP req API */
static const fwk_id_t mod_power_capping_req_scmi_api_id = FWK_ID_API_INIT(
    FWK_MODULE_IDX_SCMI_POWER_CAPPING_REQ,
    MOD_POW_CAP_REQ_API_IDX_SCMI_REQ);

/*! Identifier of the Power Managment API */
static const fwk_id_t mod_power_capping_req_power_mgmt_api_id = FWK_ID_API_INIT(
    FWK_MODULE_IDX_SCMI_POWER_CAPPING_REQ,
    MOD_POW_CAP_REQ_API_IDX_LIMITER_POWER_API);

#endif /* MOD_SCMI_POWER_CAPPING_REQ_H */
