/*
 * Arm SCP/MCP Software
 * Copyright (c) 2025, Arm Limited and Contributors. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef MOD_POSIX_TRANSPORT_H
#define MOD_POSIX_TRANSPORT_H

#include <mqueue.h>

#include <fwk_id.h>

#include <signal.h>
#include <stddef.h>
#include <stdint.h>

#ifndef POSIX_TRANSPORT_MSG_MAX_SIZE
#    define POSIX_TRANSPORT_MSG_MAX_SIZE (512)
#endif

/*!
 * \brief Channel type
 *
 * \details Defines the role of an entity in a channel
 */
enum mod_posix_transport_channel_type {
    /*! Requester channel */
    MOD_TRANSPORT_CHANNEL_TYPE_REQUESTER,
    /*! Completer channel */
    MOD_TRANSPORT_CHANNEL_TYPE_COMPLETER,
    /*! Channel type count */
    MOD_TRANSPORT_CHANNEL_TYPE_COUNT,
};

/*!
 * \brief Posix transport device configuration
 */
struct mod_posix_transport_dev_config {
    /*! Identifier of the driver */
    fwk_id_t dev_driver_id;
    /*! Identifier of the driver API to bind to */
    fwk_id_t dev_driver_api_id;
};

/*!
 * \brief Posix transport channel configuration
 */
struct mod_posix_transport_channel_config {
    /*! Configuration of the TX device */
    struct mod_posix_transport_dev_config tx_dev;
    /*! Configuration of the RX device */
    struct mod_posix_transport_dev_config rx_dev;
};

/*!
 * \brief Posix transport message
 */
struct mod_posix_transport_message {
    /*! Message size */
    size_t msg_size;
    /*! Message header (the first 4 bytes of the message) */
    uint32_t message_header;
    /*! Message payload */
    uint8_t payload[POSIX_TRANSPORT_MSG_MAX_SIZE];
};

/*!
 * \brief Posix transport device driver APIs
 */
struct mod_posix_transport_driver_api {
    /*!
     * \brief Send a message over device with the given id
     *
     * \param[in] message Pointer to the transport message to send
     * \param[in] device_id Id of the device to send the message over
     *
     * \retval ::FWK_SUCCESS The operation succeeded.
     * \return One of the standard error codes for implementation-defined
     *      errors.
     */
    int (*send_message)(
        struct mod_posix_transport_message *message,
        fwk_id_t device_id);
    /*!
     * \brief Get the current message from device with the given id
     *
     * \param[out] message Pointer to the transport message to send
     * \param[in] device_id Id of the device to send the message over
     *
     * \retval ::FWK_SUCCESS The operation succeeded.
     * \return One of the standard error codes for implementation-defined
     *      errors.
     */
    int (*get_message)(
        struct mod_posix_transport_message *message,
        fwk_id_t device_id);
};

/*!
 * \brief Driver input API (Implemented by the transport module)
 *
 * \details Interface used for driver -> Transport communication.
 */
struct mod_posix_transport_driver_input_api {
    /*!
     * \brief Signal an incoming message
     *
     * \param channel_id Channel identifier
     *
     * \retval ::FWK_SUCCESS The operation succeeded.
     * \return One of the standard error codes for implementation-defined
     *      errors.
     */
    int (*signal_message)(fwk_id_t channel_id);
};

/*!
 * \brief Posix transport APIs indexes
 */
enum mod_posix_transport_api_idx {
    MOD_POSIX_TRANSPORT_API_IDX_SCMI_TRANSPORT,
    MOD_POSIX_TRANSPORT_API_IDX_DRIVER_INPUT,
    MOD_POSIX_TRANSPORT_API_IDX_COUNT,
};

#endif /* MOD_POSIX_TRANSPORT_H */
