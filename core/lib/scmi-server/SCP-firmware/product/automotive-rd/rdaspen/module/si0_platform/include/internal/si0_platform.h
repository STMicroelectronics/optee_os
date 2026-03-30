/*
 * Arm SCP/MCP Software
 * Copyright (c) 2025, Arm Limited and Contributors. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 * Description:
 *     SCP sub-system support.
 */

#ifndef SI0_PLATFORM_H
#define SI0_PLATFORM_H

#include <mod_si0_platform.h>

#define MOD_NAME "[SI0_PLATFORM] "

/*
 * Power management interface helper functions.
 */

/*!
 * \brief Helper function to bind to power domain restricted API.
 *
 * \param None.
 *
 * \retval ::FWK_SUCCESS Operation succeeded.
 * \return One of the standard error codes for implementation-defined errors.
 */
int platform_power_mgmt_bind(void);

/*!
 * \brief Helper function to return platform system power driver API.
 *
 * \param None.
 *
 * \return Pointer to the SI0 platform system power driver API.
 */
const void *get_platform_system_power_driver_api(void);

/*
 * SCMI interface helper functions.
 */

/*!
 * \brief Helper function to return platform system SCMI API.
 *
 * \param None.
 *
 * \return Pointer to the SI0 platform system SCMI API.
 */
const void *get_platform_scmi_power_down_api(void);

#endif /* SI0_PLATFORM_H */
