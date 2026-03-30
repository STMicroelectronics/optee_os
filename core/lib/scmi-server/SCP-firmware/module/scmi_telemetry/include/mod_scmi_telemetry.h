/*
 * Arm SCP/MCP Software
 * Copyright (c) 2025, Arm Limited and Contributors. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 * Description:
 *      SCMI Telemetry Protocol Support.
 */

#ifndef MOD_SCMI_TELEMETRY_H
#define MOD_SCMI_TELEMETRY_H

#include <mod_telemetry.h>

#include <fwk_id.h>

#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>

/*!
 * \ingroup GroupModules
 * \defgroup GroupSCMI_TELEMETRY SCMI Telemetry Management Protocol
 * \{
 */

/*!
 * \brief Permission flags governing the ability to use certain SCMI commands
 *     to interact with a telemetry device.
 */

/*! No permissions (at least one must be granted) */
#define MOD_SCMI_TELEMETRY_PERM_INVALID 0UL

/*! The telemetry device's attributes can be queried */
#define MOD_SCMI_TELEMETRY_PERM_ATTRIBUTES (1U << 0)

/*! The permission to access telemetry data */
#define MOD_SCMI_TELEMETRY_PERM_TELEMETRY (1U << 1)

/*!
 * \brief Telemetry device structure.
 *
 * \details This structure is used in per-agent telemetry device tables.
 * Each device contains an identifier for an element that implements the
 * telemetry API.
 */
struct mod_scmi_telemetry_device {
    /*! Telemetry element identifier */
    fwk_id_t telemetry_source_id;

    /*! Permissions mask (defined by telemetry device configuration) */
    uint8_t permissions;
};

/*!
 * \brief Agent specific information for Telemetry SHMTI.
 *
 * \details Describes the telemetry shmti region and its access information for
 * each agent.
 */
struct mod_scmi_telemetry_agent_shmti_config {
    /*! SHMTI ID. */
    uint32_t shmti_id;

    /*! Start addresses of SHMTI region for the agent. */
    const uint64_t start_addr;
};

/*!
 * \brief SCMI Agent descriptor.
 *
 * \details Describes an SCMI agent using the telemetry protocol.
 * It provides a list of accessible telemetry devices.
 */
struct mod_scmi_telemetry_agent {
    /*! Pointer to telemetry device table for this agent */
    const struct mod_scmi_telemetry_device *device_table;

    /*! Number of devices in the agent's telemetry table */
    uint8_t agent_device_count;

    /*! Number of SHMTI regions visible to this agent. */
    uint32_t shmti_count;

    /*! List of SHMTI regions for the agent. */
    const struct mod_scmi_telemetry_agent_shmti_config *shmtis;
};

/*!
 * \brief SCMI Telemetry module configuration.
 */
struct mod_scmi_telemetry_config {
    /*!
     * \brief Pointer to agent descriptors table.
     * Each entry provides a per-agent view of telemetry devices.
     */
    const struct mod_scmi_telemetry_agent *agent_table;

    /*! Number of agents in the system */
    uint32_t agent_count;

    /*! Maximum number of DEs that can be enabled at once. */
    uint32_t max_enabled_de_count;

    /*! Number of available Telemetry Shared memory areas (SHMTI)  */
    uint32_t shmti_count;

    /*! List of available Telemetry Shared memory areas (SHMTI)  */
    const struct mod_telemetry_shmti_info *shmti_list;

    /*! Platform supports single-sample asynchronous telemetry reads. */
    bool single_sample_async_read_support;

    /*! Platform supports continuous update notifications. */
    bool continuous_update_notification_support;

    /*!
     * \brief Platform supports per event-group sampling rate
     * and collection mode configuration.
     */
    bool event_group_sampling_config_support;

    /*! Platform supports telemetry reset flow. */
    bool telemetry_reset_support;

    /*! Platform supports Telemetry FastChannels. */
    bool fastchannel_support;
};

/*!
 * \brief SCMI Telemetry API indices.
 */
enum scmi_telemetry_api_idx {
    /*! Index for the SCMI protocol API */
    MOD_SCMI_TELEMETRY_PROTOCOL_API,

    /*! Number of APIs */
    MOD_SCMI_TELEMETRY_API_COUNT
};

/*!
 * \defgroup GroupScmiTelemetryPolicyHandler SCMI Telemetry Policy Handler
 * \brief Policy handlers for SCMI Telemetry.
 *
 * \details SCMI policy handlers allow platforms to enforce specific policies.
 * These handlers may be overridden in platform-specific implementations.
 *
 * \{
 */

/*!
 * \brief Policy handler return values.
 *
 * \details These values determine whether the SCMI message handler should
 * proceed with processing or reject the request.
 */
enum mod_scmi_telemetry_policy_status {
    /*! Reject the request */
    MOD_SCMI_TELEMETRY_SKIP_MESSAGE_HANDLER,

    /*! Process the request */
    MOD_SCMI_TELEMETRY_EXECUTE_MESSAGE_HANDLER,
};

/*!
 * \brief SCMI Telemetry Request Policy.
 *
 * \details This function determines whether the telemetry request should be
 * granted or rejected. The policy handler is executed before message
 * processing.
 *
 * \param[out] policy_status  Resulting policy status.
 * \param[in] agent_id  Identifier of the requesting SCMI agent.
 * \param[in] device_id  Identifier of the telemetry device.
 *
 * \retval ::FWK_SUCCESS Operation succeeded.
 * \return One of the standard framework error codes.
 */
int scmi_telemetry_telemetry_request_policy(
    enum mod_scmi_telemetry_policy_status *policy_status,
    uint32_t agent_id,
    uint32_t device_id);

/*!
 * \}
 */

/*!
 * \}
 */

#endif /* MOD_SCMI_TELEMETRY_H */
