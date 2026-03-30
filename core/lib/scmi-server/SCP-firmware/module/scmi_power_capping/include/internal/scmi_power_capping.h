/*
 * Arm SCP/MCP Software
 * Copyright (c) 2023-2025, Arm Limited and Contributors. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 * Description:
 *      SCMI power capping and monitoring protocol completer support.
 */

#ifndef INTERNAL_SCMI_POWER_CAPPING_H
#define INTERNAL_SCMI_POWER_CAPPING_H

#include "mod_power_capping.h"
#include "mod_scmi_power_capping.h"

#include <fwk_event.h>
#include <fwk_id.h>

#include <stdint.h>

/*!
 * \addtogroup GroupModules Modules
 * \{
 */

/*!
 * \defgroup GroupSCMI_POWER_CAPPING SCMI power capping and monitoring Protocol
 * \{
 */

/*
 * SCMI power cap event IDs
 */
enum scmi_power_capping_event_idx {
    SCMI_POWER_CAPPING_EVENT_IDX_PROCESS_HAL_CAP_PAI_NOTIFICATION,
#ifdef BUILD_HAS_SCMI_NOTIFICATIONS
    SCMI_POWER_CAPPING_EVENT_IDX_PROCESS_HAL_MEASUREMENT_NOTIF,
#endif
#ifdef BUILD_HAS_SCMI_POWER_CAPPING_FAST_CHANNELS_COMMANDS
    SCMI_POWER_CAPPING_EVENT_IDX_FAST_CHANNELS_PROCESS,
#endif
    SCMI_POWER_CAPPING_EVENT_COUNT,
};

/*!
 * \brief Power management related APIs.
 */
struct mod_scmi_power_capping_power_apis {
    /* Power capping API */
    const struct mod_power_capping_api *power_capping_api;
};

struct mod_scmi_power_capping_domain_context {
    /* Power capping domain configuration */
    const struct mod_scmi_power_capping_domain_config *config;

    /*!
     * \brief Power Cap Service ID
     *
     * \details Stores the service id corresponding to the agent that requested
     *      a power cap indicating that the domain is busy.
     *      It is set to FWK_ID_NONE when the domain is not used by an agent.
     */

    fwk_id_t cap_pending_service_id;

    /*!
     * \brief Power Cap Notification Service ID
     *
     * \details Stores the service id corresponding to the agent that requested
     *      a Power Cap change. This variable is then used to determine which
     *      agent that is responsible for the Power Cap change SCMI
     *      notification.
     *      It is set to FWK_ID_NONE when no agent is responsible for the Power
     *      Cap change notification.
     */
    fwk_id_t cap_notification_service_id;

    /*!
     * \brief PAI Notification Service ID
     *
     * \details Stores the service id corresponding to the agent that requested
     *      a PAI change. This variable is then used to determine which agent
     *      that is responsible for the PAI change SCMI notification.
     *      It is set to FWK_ID_NONE when no agent is responsible for the PAI
     *      change notification.
     */
    fwk_id_t pai_notification_service_id;

    /*!
     * \brief Power capping configuration support.
     */
    bool cap_config_support;

    /*!
     * \brief Indicates if the cap request is asynchronous or not.
     */
    bool is_cap_request_async;
};

struct mod_scmi_power_capping_context {
    /* Table of power capping domain ctxs */
    struct mod_scmi_power_capping_domain_context
        *power_capping_domain_ctx_table;
    /* Power capping domain count */
    uint32_t domain_count;
};

/*!
 * \}
 */

/*!
 * \}
 */

#endif /* INTERNAL_SCMI_POWER_CAPPING_H */
