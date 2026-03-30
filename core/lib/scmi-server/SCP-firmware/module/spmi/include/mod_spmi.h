/*
 * Arm SCP/MCP Software
 * Copyright (c) 2024-2025, Arm Limited and Contributors. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 * Description:
 *     SPMI HAL module - a module requiring access to peripheral through
 *     the SPMI bus can bind to this module
 */

#ifndef MOD_SPMI_H
#define MOD_SPMI_H

#include <fwk_assert.h>
#include <fwk_element.h>
#include <fwk_event.h>
#include <fwk_id.h>
#include <fwk_module_idx.h>

#include <stdbool.h>
#include <stdint.h>

/*!
 * \addtogroup GroupModules Modules
 * \{
 */

/*!
 * \defgroup GroupSPMI SPMI HAL
 *
 * \details Support for transmitting and receiving data through SPMI Bus
 * \{
 */

/*!
 * \brief Configuration data for an SPMI device
 */
struct mod_spmi_dev_config {
    /*!
     *  Identifier of the element of the SPMI controller driver corresponding
     *  to this device
     */
    fwk_id_t driver_id;

    /*!
     *   Identifier of the driver API
     */
    fwk_id_t api_id;
};

/*!
 * \brief Parameters of the SPMI event sent from
 *        driver to HAL
 */
struct mod_spmi_event_param {
    /*!
     *   Status of the SPMI transaction
     */
    int status;
};

/*!
 * \brief SPMI Commands
 */
enum mod_spmi_command {
    MOD_SPMI_CMD_EXT_REG_WRITE,
    MOD_SPMI_CMD_RESET,
    MOD_SPMI_CMD_SLEEP,
    MOD_SPMI_CMD_SHUTDOWN,
    MOD_SPMI_CMD_WAKEUP,
    MOD_SPMI_CMD_AUTHENTICATE,
    MOD_SPMI_CMD_MASTER_READ,
    MOD_SPMI_CMD_MASTER_WRITE,
    MOD_SPMI_CMD_TBO,
    MOD_SPMI_CMD_DDB_MASTER_READ,
    MOD_SPMI_CMD_DDB_SLAVE_READ,
    MOD_SPMI_CMD_EXT_REG_READ,
    MOD_SPMI_CMD_EXT_REG_WRITE_LONG,
    MOD_SPMI_CMD_EXT_REG_READ_LONG,
    MOD_SPMI_CMD_REG_WRITE,
    MOD_SPMI_CMD_REG_READ,
    MOD_SPMI_CMD_REG0_WRITE,
};

/*!
 * \brief  SPMI transaction request parameters
 */
struct mod_spmi_request {
    /*!
     *\brief SPMI command
     */
    enum mod_spmi_command command;

    /*!
     * \brief SPMI data byte count
     */
    uint8_t byte_count;

    /*!
     * \brief ID of the target requester/completer on SPMI bus
     */
    uint8_t target_id;

    /*!
     * \brief Register Address
     */
    uint32_t reg_address;

    /*!
     * \brief Pointer to data to be transmitted/received
     */
    uint8_t *data;
};

/*!
 * \brief SPMI driver interface.
 *
 * \details The interface the SPMI HAL module relies on
 *          to perform actions on an SPMI device.
 */
struct mod_spmi_driver_api {
    /*!
     * \brief Send a SPMI command as SPMI requester
     *
     * \details When the function returns the command may not be completed.
     *          The driver can assume the integrity of the data during the
     *          transmission. When the transmission operation has finished,
     *          the driver shall report it through the SPMI HAL module driver
     *          response API
     *
     * \param dev_id       Identifier of the SPMI device
     * \param request      Information for the SPMI command to be sent
     *
     * \retval ::FWK_PENDING    The request was submitted
     * \retval ::FWK_SUCCESS    The request was successfully completed
     * \retval ::FWK_E_PARAM    One or more parameters were invalid
     * \return One of the standard framework status codes
     */
    int (*send_command)(fwk_id_t dev_id, struct mod_spmi_request *request);
};

/*!
 * \brief SPMI HAL module interface.
 */
struct mod_spmi_api {
    /*!
     * \brief Request read of data as SPMI requester from a SPMI completer
     *
     * \details When the function returns the reception is not completed,
     *          possibly not even started. The data buffer must stay allocated
     *          and its content must not be modified until the reception is
     *          completed or aborted. When the reception operation has finished
     *          a response event is sent to the client.
     *
     * \param dev_id        Identifier of the SPMI device
     * \param target_id     4-bit address of the target on the SPMI bus
     * \param reg_addr      16-bit address of the register within SPMI completer
     * \param data          Pointer to the buffer to receive data from target
     * \param byte_count    Number of data bytes to receive
     *
     * \retval ::FWK_PENDING    The request was submitted
     * \retval ::FWK_E_PARAM    One or more parameters were invalid
     * \retval ::FWK_E_BUSY     An SPMI transaction is already on-going
     * \retval ::FWK_E_DEVICE   The reception is aborted due to a device error
     * \return One of the standard framework status codes
     */
    int (*completer_read)(
        fwk_id_t dev_id,
        uint8_t target_id,
        uint32_t reg_addr,
        uint8_t *data,
        uint8_t byte_count);

    /*!
     * \brief Request write of data as SPMI requester to a SPMI completer
     *
     * \details When the function returns the transmission is not completed,
     *          possibly not even started. The data buffer must stay allocated
     *          and its content must not be modified until the reception is
     *          completed or aborted. When the reception operation has finished
     *          a response event is sent to the client.
     *
     * \param dev_id        Identifier of the SPMI device
     * \param target_id     4-bit address of the target on the SPMI bus
     * \param reg_addr      16-bit address of the register within SPMI completer
     * \param data          Pointer to the buffer to send data to the target
     * \param byte_count    Number of data bytes to send
     *
     * \retval ::FWK_PENDING    The request was submitted.
     * \retval ::FWK_E_PARAM    One or more parameters were invalid.
     * \retval ::FWK_E_BUSY     An SPMI transaction is already on-going.
     * \retval ::FWK_E_DEVICE   The reception is aborted due to a device error
     * \return One of the standard framework status codes.
     */
    int (*completer_write)(
        fwk_id_t dev_id,
        uint8_t target_id,
        uint32_t reg_addr,
        uint8_t *data,
        uint8_t byte_count);

    /*!
     * \brief Send a power command as SPMI requester to a SPMI completer
     *
     * \details When the function returns the transmission is not completed,
     *          possibly not even started. When the transmission operation has
     *          finished a response event is sent to the client.
     *
     * \param dev_id        Identifier of the SPMI device
     * \param target_id     4-bit address of the target on the SPMI bus
     * \param command       Power operation to send to the target
     *
     * \retval ::FWK_PENDING    The request was submitted.
     * \retval ::FWK_E_PARAM    One or more parameters were invalid.
     * \retval ::FWK_E_BUSY     An SPMI transaction is already on-going.
     * \retval ::FWK_E_DEVICE   The reception is aborted due to a device error
     * \return One of the standard framework status codes.
     */
    int (*power_operation)(
        fwk_id_t dev_id,
        uint8_t target_id,
        enum mod_spmi_command command);

    /*!
     * \brief Send a authentication command as SPMI requester to a SPMI
     * completer
     *
     * \details When the function returns the transmission is not completed,
     *          possibly not even started. When the transmission operation has
     *          finished a response event is sent to the client.
     *
     * \param dev_id        Identifier of the SPMI device
     * \param target_id     4-bit address of the target on the SPMI bus
     * \param data          Buffer to hold authentication data
     * \param byte_count    Number of data bytes to send
     *
     * \retval ::FWK_PENDING The request was submitted.
     * \retval ::FWK_E_PARAM One or more parameters were invalid.
     * \retval ::FWK_E_BUSY An SPMI transaction is already on-going.
     * \retval ::FWK_E_DEVICE The reception is aborted due to a device error
     * \return One of the standard framework status codes.
     */
    int (*authenticate)(
        fwk_id_t dev_id,
        uint8_t target_id,
        uint8_t *data,
        uint8_t byte_count);

    /*!
     * \brief Request read of data as SPMI requester from another requester
     *
     * \details When the function returns the reception is not completed,
     *          possibly not even started. The data buffer must stay allocated
     *          and its content must not be modified until the reception is
     *          completed or aborted. When the reception operation has finished
     *          a response event is sent to the client.
     *
     * \param dev_id      Identifier of the SPMI device
     * \param target_id   4-bit address of the target on the SPMI bus
     * \param reg_addr    16-bit address of the register within SPMI requester
     * \param data        Pointer to the buffer to receive data from the target
     *
     * \retval ::FWK_PENDING    The request was submitted.
     * \retval ::FWK_E_PARAM    One or more parameters were invalid.
     * \retval ::FWK_E_BUSY     An SPMI transaction is already on-going.
     * \retval ::FWK_E_DEVICE   The reception is aborted due to a device error
     * \return One of the standard framework status codes.
     */
    int (*requester_read)(
        fwk_id_t dev_id,
        uint8_t target_id,
        uint32_t reg_addr,
        uint8_t *data);

    /*!
     * \brief Request write of data as SPMI requester to another requester
     *
     * \details When the function returns the transmission is not completed,
     *          possibly not even started. The data buffer must stay allocated
     *          and its content must not be modified until the transmission is
     *          completed or aborted. When the transmission operation has
     *          finished a response event is sent to the client.
     *
     * \param dev_id        Identifier of the SPMI device
     * \param target_id     4-bit address of the target on the SPMI bus
     * \param reg_addr      16-bit address of the register within SPMI requester
     * \param data          Pointer to the buffer to send data to the target
     *
     * \retval ::FWK_PENDING    The request was submitted.
     * \retval ::FWK_E_PARAM    One or more parameters were invalid.
     * \retval ::FWK_E_BUSY     An SPMI transaction is already on-going.
     * \retval ::FWK_E_DEVICE   The reception is aborted due to a device error
     * \return One of the standard framework status codes.
     */
    int (*requester_write)(
        fwk_id_t dev_id,
        uint8_t target_id,
        uint32_t reg_addr,
        uint8_t *data);

    /*!
     * \brief Read Device Descriptor Block as SPMI requester
     *        from another SPMI requester or completer
     *
     * \details When the function returns the reception is not completed,
     *          possibly not even started. The data buffer must stay allocated
     *          and its content must not be modified until the reception is
     *          completed or aborted. When the reception operation has finished
     *          a response event is sent to the client.
     *
     * \param dev_id        Identifier of the SPMI device
     * \param target_id     4-bit address of the target on the SPMI bus
     * \param is_requester  Is the target SPMI device requester or completer
     * \param data          Pointer to the buffer to hold the DDB data
     *
     * \retval ::FWK_PENDING    The request was submitted.
     * \retval ::FWK_E_PARAM    One or more parameters were invalid.
     * \retval ::FWK_E_BUSY     An SPMI transaction is already on-going.
     * \retval ::FWK_E_DEVICE   The reception is aborted due to a device error
     * \return One of the standard framework status codes.
     */
    int (*device_descriptor_block_read)(
        fwk_id_t dev_id,
        uint8_t target_id,
        bool is_requester,
        uint8_t *data);
};

/*!
 * \brief   SPMI HAL module driver response API.
 *
 * \details The interface the SPMI HAL module exposes to its module drivers to
 *          report reception from Request Capable Completer. In future this API
 *          can be extended to report other events from the driver if required.
 */
struct mod_spmi_driver_response_api {
    /*!
     * \brief Function called back after the completion or abortion of an SPMI
     *        transaction request
     *
     * \param dev_id Identifier of the SPMI device
     * \param spmi_status SPMI transaction status
     */
    void (*transaction_completed)(fwk_id_t dev_id, int spmi_status);
};

/*!
 * \defgroup GroupSPMIIds Identifiers
 * \{
 */

/*!
 * \brief API indices.
 */
enum mod_spmi_api_idx {
    /*! APIs to send SPMI commands - to be used by the PSU driver module */
    MOD_SPMI_API_IDX_SPMI,

    /*! API used by SPMI controller driver module to send driver response */
    MOD_SPMI_API_IDX_DRIVER_RESPONSE,

    /*! Number of exposed interfaces */
    MOD_SPMI_API_IDX_COUNT,
};

/*! SPMI API identifier */
extern const fwk_id_t mod_spmi_api_id_spmi;
/*! Driver response API identifier */
extern const fwk_id_t mod_spmi_api_id_driver_response;

/*!
 * \}
 */

/*!
 * \}
 */

/*!
 * \}
 */

#endif /* MOD_SPMI_H */
