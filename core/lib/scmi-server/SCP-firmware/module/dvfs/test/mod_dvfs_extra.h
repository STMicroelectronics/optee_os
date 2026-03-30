/*
 * Arm SCP/MCP Software
 * Copyright (c) 2024, Arm Limited and Contributors. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 * Description:
 *      System Control and Management Interface (SCMI) unit test support.
 */
#include <mod_clock.h>
#include <mod_dvfs.h>
#include <mod_scmi.h>

#include <fwk_event.h>

/*!
 * \brief Set the voltage of a device.
 *
 * \param[in] device_id Identifier of the device to set the voltage of.
 * \param[in] voltage New voltage in millivolts (mV).
 *
 * \retval ::FWK_E_HANDLER An error occurred in the device driver.
 * \retval ::FWK_E_PARAM One or more parameters were invalid.
 * \retval ::FWK_E_STATE The device cannot currently accept the request.
 * \retval ::FWK_SUCCESS The operation succeeded.
 *
 * \return Status code representing the result of the operation.
 */
int mod_psu_device_api_set_voltage(fwk_id_t device_id, uint32_t voltage);

/*!
 * \brief Set a new clock rate by providing a frequency in Hertz (Hz).
 *
 * \param clock_id Clock device identifier.
 *
 * \param rate The desired frequency in Hertz.
 *
 * \param round_mode The type of rounding to perform, if required, to
 *      achieve the given rate.
 *
 * \param requester_id The entity which requested the operation.
 *
 * \retval ::FWK_SUCCESS The operation succeeded.
 * \retval ::FWK_PENDING The request is pending. The result for this
 *      operation will be provided via a response event.
 * \retval ::FWK_E_PARAM The clock identifier was invalid.
 * \retval ::FWK_E_SUPPORT Deferred handling of asynchronous drivers is not
 *      supported.
 * \return One of the standard framework error codes.
 */
int mod_clock_api_set_rate(
    fwk_id_t clock_id,
    uint64_t rate,
    enum mod_clock_round_mode round_mode,
    unsigned int requester_id);
