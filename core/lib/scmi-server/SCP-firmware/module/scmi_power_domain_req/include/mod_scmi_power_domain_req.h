/*
 * Arm SCP/MCP Software
 * Copyright (c) 2024, Arm Limited and Contributors. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef MOD_SCMI_POWER_DOMAIN_REQ_H
#define MOD_SCMI_POWER_DOMAIN_REQ_H

#include <fwk_id.h>
#include <fwk_macros.h>
#include <fwk_module_idx.h>

#include <stddef.h>
#include <stdint.h>

/*!
 * \brief Power domain requester device configuration
 */
struct scmi_power_domain_req_dev_config {
    /*!
     * \brief Start state after initialization of the external pds
     */
    uint32_t start_state;

    /*!
     * \brief Element ID in Power Domain config
     */
    fwk_id_t pd_element_id;

    /*!
     * \brief Timer ID
     */
    fwk_id_t timer_id;

    /*!
     * \brief Timeout used for the alarm
     */
    uint32_t response_timeout;

    /*!
     * \brief Domain ID in Power Domain config
     */
    uint32_t domain_id;

    /*!
     * \brief Channel Identifier
     *
     * \details The service ID which corresponds to the required
     *          channel in the transport layer.
     *
     */
    fwk_id_t service_id;

    /*!
     * \brief Asynchronous or Synchronus Request
     *
     * \details The sync flag is used to determine wether we need
     *          to send Synchronus or Asynchronus request
     *
     */
    uint32_t sync_flag;
};

/*!
 * \brief Power domain request payload
 */
struct scmi_pd_req_power_state_set_a2p {
    /*! Request Flags */
    uint32_t flags;

    /*! Domain ID in the platform */
    uint32_t domain_id;

    /*! Requested power state */
    uint32_t power_state;
};

/*!
 * \brief Power domain requester response payload
 */
struct scmi_pd_req_power_state_set_p2a {
    /*! Response status of the request */
    int32_t status;
};

/*!
 * \brief API indices
 */
enum mod_power_domain_req_api_idx {
    /*! API used for sending SCMI commands and receive responses */
    MOD_POW_DOMAIN_REQ_API_IDX_SCMI_REQ,

    /*! API used to set and get the state from another module */
    MOD_POW_DOMAIN_REQ_API_IDX_REQ,

    MOD_POW_DOMAIN_REQ_API_IDX_COUNT,
};

/*! Identifier of the system power req API */
extern const fwk_id_t mod_power_domain_req_api_id;

/*! Identifier of the system power req API */
extern const fwk_id_t mod_power_domain_req_scmi_api_id;

#endif /* MOD_SCMI_POWER_DOMAIN_REQ_H */
