/*
 * Arm SCP/MCP Software
 * Copyright (c) 2024, Arm Limited and Contributors. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 * Description:
 *      SPMI unit test header to generate mock APIs for driver
 */

#ifndef MOD_SPMI_EXTRA_H
#define MOD_SPMI_EXTRA_H

#include <mod_spmi.h>

/*!
 * \brief (UNIT Tests) SPMI module to SPMI Controller driver API
 */

/*!
 * \brief Send a SPMI command as SPMI requester
 *
 * \details When the function returns the read may not be completed.
 *          The driver can assume the integrity of the data during the
 *          transmission. When the transmission operation has finished,
 *          the driver shall report it through the SPMI HAL module driver
 *          response API
 *
 * \param dev_id       Identifier of the SPMI device
 * \param request      Request information for the SPMI command
 *
 * \retval ::FWK_PENDING    The request was submitted
 * \retval ::FWK_SUCCESS    The request was successfully completed
 * \retval ::FWK_E_PARAM    One or more parameters were invalid
 * \return One of the standard framework status codes
 */
int mod_spmi_driver_send_command(
    fwk_id_t dev_id,
    struct mod_spmi_request *request);

#endif /* MOD_SPMI_EXTRA_H */
