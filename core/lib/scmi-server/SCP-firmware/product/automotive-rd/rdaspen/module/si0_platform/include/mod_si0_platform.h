/*
 * Arm SCP/MCP Software
 * Copyright (c) 2025, Arm Limited and Contributors. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 * Description:
 *     SCP Platform Support
 */

#ifndef MOD_SI0_PLATFORM_H
#define MOD_SI0_PLATFORM_H

#include <fwk_id.h>
#include <fwk_module_idx.h>

#include <stdint.h>

/*!
 * \addtogroup GroupPLATFORMModule PLATFORM Product Modules
 * @{
 */

/*!
 * \defgroup GroupSI0Platform SI0 Platform Support
 * @{
 */

/*!
 * \brief Indices of the interfaces exposed by the module.
 */
enum mod_si0_platform_api_idx {
    /*! API index for the powerdown interface of SCMI module */
    MOD_SI0_PLATFORM_API_IDX_SCMI_POWER_DOWN,
    /*! API index for the driver interface of the SYSTEM POWER module */
    MOD_SI0_PLATFORM_API_IDX_SYSTEM_POWER_DRIVER,
    /*! Number of exposed interfaces */
    MOD_SI0_PLATFORM_API_COUNT
};

/*!
 * \brief Notification indices.
 */
enum mod_si0_platform_notification_idx {
    /*! SI0 subsystem initialization completion notification */
    MOD_SI0_PLATFORM_NOTIFICATION_IDX_SUBSYS_INITIALIZED,

    /*! Number of notifications defined by the module */
    MOD_SI0_PLATFORM_NOTIFICATION_COUNT,
};

/*!
 * \brief Identifier for the
 * ::MOD_SI0_PLATFORM_NOTIFICATION_IDX_SUBSYS_INITIALIZED notification.
 */
static const fwk_id_t mod_si0_platform_notification_subsys_init =
    FWK_ID_NOTIFICATION_INIT(
        FWK_MODULE_IDX_SI0_PLATFORM,
        MOD_SI0_PLATFORM_NOTIFICATION_IDX_SUBSYS_INITIALIZED);

/*!
 * @}
 */

/*!
 * @}
 */

#endif /* MOD_SI0_PLATFORM_H */
