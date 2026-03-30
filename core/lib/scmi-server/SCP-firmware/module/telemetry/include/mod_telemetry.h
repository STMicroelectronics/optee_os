/*
 * Arm SCP/MCP Software
 * Copyright (c) 2025, Arm Limited and Contributors. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 * Description:
 *     Telemetry domain HAL (Hardware Abstraction Layer).
 *
 *     This module provides telemetry functionalities, including memory
 *     management for SHMTI (Shared Memory Telemetry Interface), bitmaps for
 *     allocation tracking, and functions for telemetry source and event
 *     management.
 */
#ifndef MOD_TELEMETRY_H
#define MOD_TELEMETRY_H

#include <fwk_id.h>
#include <fwk_macros.h>

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

/*!
 * \addtogroup GroupModules Modules
 * \{
 */

/*!
 * \defgroup GroupTELEMETRY Telemetry HAL.
 * \{
 */

/*! Size(Bytes) of the LINE with Timestamp. */
#define MOD_TELEMETRY_DE_LINE_SIZE_TS (24)
/*! Size(Bytes) of the LINE without Timestamp. */
#define MOD_TELEMETRY_DE_LINE_SIZE_NON_TS (16)
/*! Size(Bytes) of the LINE with Block Timestamp. */
#define MOD_TELEMETRY_DE_BLOCK_LINE_SIZE (16)
/*! Size(Bytes) of the Timestamp field in a line. */
#define MOD_TELEMETRY_TS_N_BYTES (8)
/*! Size(Bytes) of the Data field in a line. */
#define MOD_TELEMETRY_DE_DATA_SIZE_PER_LINE (8)

/*!
 * \brief Telemetry HAL Data event handle.
 */
typedef struct {
    /*! Flag to indicate if handle is valid. */
    uint32_t valid : 8;

    /*! Source Index */
    uint32_t source_index : 8;

    /*! Index of the group within the source. */
    uint32_t group_index : 8;

    /*! Index of the DE within the group. */
    uint32_t de_offset_index : 8;
} telemetry_de_handle_st;

/*!
 * \brief Holds SHMTI Information.
 */
struct mod_telemetry_shmti_info {
    /*! ID of this SHMTI region */
    uint32_t shmti_id;
    /*! Start address of the SHMTI region. */
    uintptr_t start_addr;
    /*! Length of the SHMTI region. */
    size_t length;
};

/*!
 * \brief Telemetry update interval format types.
 */
enum mod_telemetry_update_interval_formats {
    MOD_TELEMETRY_UPDATE_INTERVALS_DISCRETE,
    MOD_TELEMETRY_UPDATE_INTERVALS_LINEAR
};

/*!
 * \brief Holds Telemetry module element config.
 *
 * Element of Telemetry module is a Telemetry source which is responsible for
 * providing data for DEs and Event groups.
 */
struct mod_telemetry_source_config {
    /*! Identifier of the API ID. */
    fwk_id_t drv_api_id;

    /*! Uses Block Timestamp. */
    bool uses_blk_ts;
};

/*!
 * \brief Holds Telemetry module config.
 */
struct mod_telemetry_config {
    /*! Identifier for the Telemetry update data alarm. */
    fwk_id_t alarm_id;
    /*! Number of valid update intervals available. */
    uint32_t num_intervals;
    /*! Telemetry update interval type. */
    enum mod_telemetry_update_interval_formats interval_format;
    /*! List of available sampling rates. */
    uint32_t *sampling_rates;
};

/*!
 * \brief Telemetry Data Event Fast Channel Attributes.
 */
struct mod_telemetry_de_fch_attr {
    /*! Lower 32 bits of fast channel address. */
    uint32_t fch_addr_low;
    /*! Higher 32 bits of fast channel address. */
    uint32_t fch_addr_high;
    /*! Size of fast channel in bytes. */
    uint32_t fch_size;
};

/*!
 * \brief Telemetry Data Event Description.
 */
struct mod_telemetry_de_desc {
    /*! Data event ID. */
    uint32_t de_id;
    /*! Group ID. */
    uint32_t group_id;
    /*! Size of the data event in bytes. */
    uint32_t de_data_size;
    /*! Data Event mandatory attributes. */
    uint32_t attributes[3];
};

/*!
 * \brief Telemetry Data Event status.
 */
struct mod_telemetry_de_status {
    /*! Data event ID. */
    uint32_t de_id;
    /*! Flag to indicate if timestamp is enabled. */
    bool ts_enabled;
};

/*!
 * \brief Stores DE's telemetry data.
 */
struct mod_telemetry_de_data {
    /*! Index of the Event in the source. */
    uint32_t source_de_index;
    /*! Number of QWORDS in data. */
    size_t data_qword_count;
    /*! Data associated with Event. */
    uint64_t *data;
    /*! Timestamp for the data. */
    uint64_t timestamp;
};
/*!
 * \brief Telemetry HAL API indices.
 */
enum mod_telemetry_api_idx {
    MOD_TELEMETRY_API_IDX_PROTOCOL_SUPPORT,
    MOD_TELEMETRY_API_IDX_DRIVER_SUPPORT,
    MOD_TELEMETRY_DRIVER_API_IDX_HAL_SUPPORT,
    MOD_TELEMETRY_API_IDX_COUNT
};

/*!
 * \brief Telemetry Driver API.
 *
 * This API allows telemetry source drivers to register data events,
 * enable/disable them, and update telemetry data.
 */
struct mod_telemetry_driver_api {
    /*!
     * \brief Get the list of registered Data Events (DEs).
     *
     * \param[out] num_de Pointer to store the number of DEs.
     * \param[out] de_list Pointer to store the DE list.
     *
     * \retval ::FWK_SUCCESS The operation was successful.
     * \return One of the standard framework error codes.
     */
    int (*get_de_list)(
        uint32_t *num_de,
        const struct mod_telemetry_de_desc **de_list);

    /*!
     * \brief Get the additionnal fast channel attributes for Data Events (DEs).
     *
     * \param[in] de_index Index of the DE in the source.
     * \param[out] de_fch_attr Pointer to store the fast Channel attributes.
     *
     * \retval ::FWK_SUCCESS The operation was successful.
     * \return One of the standard framework error codes.
     */
    int (*get_de_fch_attr)(
        uint32_t de_index,
        struct mod_telemetry_de_fch_attr *de_fch_attr);

    /*!
     * \brief Retrieve name of a specific Data Event (DE).
     *
     * \param[in] de_index Index of the DE in the source.
     * \param[out] name Pointer to store the DE name.
     *
     * \retval ::FWK_SUCCESS The operation was successful.
     * \return One of the standard framework error codes.
     */
    int (*get_de_name)(uint32_t de_index, char *name);

    /*!
     * \brief Disable a specific Data Event (DE).
     *
     * \param[in] de_index Index of the DE to disable.
     *
     * \retval ::FWK_SUCCESS The operation was successful.
     * \return One of the standard framework error codes.
     */
    int (*disable_de)(uint32_t de_index);

    /*!
     * \brief Disable all Data Events (DEs).
     *
     * \retval ::FWK_SUCCESS The operation was successful.
     * \return One of the standard framework error codes.
     */
    int (*disable_de_all)(void);

    /*!
     * \brief Enable a Data Event (DE) without an individual timestamp.
     *
     * \param[in] de_index Index of the DE to enable.
     * \param[out] shmti_id SHMTI ID contianing the give Data Event.
     * \param[out] shmti_de_offset Byte offset from the start of the given
     *
     * \retval ::FWK_SUCCESS The operation was successful.
     * \return One of the standard framework error codes.
     */
    int (*enable_de)(uint32_t de_index);

    /*!
     * \brief Fetch the data from source driver.
     *
     * This function fetches the data from the source driver when it's ready.
     * registered telemetry sources.
     *
     * \param[out] data_array Array of Data.
     * \param[out] num_des Number of DEs available for consumption.
     *
     * \retval ::FWK_SUCCESS The operation was successful.
     * \return One of the standard framework error codes.

     */
    int (
        *get_data)(struct mod_telemetry_de_data **data_array, uint32_t num_des);
};

/*!
 * \brief Telemetry Protocol Support API.
 *
 * This API provides protocol-level operations for managing telemetry
 * sources, data events, and configuration settings.
 */
struct mod_telemetry_protocol_support_api {
    /*!
     * \brief Initialise the SHMTI(s) .
     *
     * \param[in] config Pointer to telemetry config container.
     * \param[out] shmti_ctx_list List of initialised SHMTI contexts.
     *
     * \retval ::FWK_SUCCESS The operation was successful.
     * \return One of the standard framework error codes.
     */
    int (*shmti_init)(
        const struct mod_telemetry_shmti_info *shmti_info,
        uint32_t shmti_count);

    /*!
     * \brief Get the total number of registered Data Events (DEs).
     *
     * \param[out] num_de Pointer to store the number of DEs.
     *
     * \retval ::FWK_SUCCESS The operation was successful.
     * \return One of the standard framework error codes.
     */
    int (*get_num_de)(uint32_t *num_de);

    /*!
     * \brief Retrieve description of a specific Data Event (DE).
     *
     * \param[in] de_index Index of the DE.
     * \param[out] de_desc Pointer to store the Data Event descriptor.
     *
     * \retval ::FWK_SUCCESS The operation was successful.
     * \return One of the standard framework error codes.
     */
    int (
        *get_de_desc)(uint32_t de_index, struct mod_telemetry_de_desc *de_desc);

    /*!
     * \brief Retrieve Fast Channel details of a specific Data Event (DE).
     *
     * \param[in] de_index Index of the DE.
     * \param[out] de_fch_attr Pointer to store the fast Channel attributes.
     *
     * \retval ::FWK_SUCCESS The operation was successful.
     * \return One of the standard framework error codes.
     */
    int (*get_de_fch_desc)(
        uint32_t de_index,
        struct mod_telemetry_de_fch_attr *de_fch_attr);

    /*!
     * \brief Retrieve name of a specific Data Event (DE).
     *
     * \param[in] de_index Index of the DE.
     * \param[out] name Pointer to store the DE name.
     *
     * \retval ::FWK_SUCCESS The operation was successful.
     * \return One of the standard framework error codes.
     */
    int (*get_de_name)(uint32_t de_index, char *name);

    /*!
     * \brief Get information about supported update intervals.
     *
     * \param[out] num_intervals Pointer to store the number of intervals.
     * \param[out] interval_format Pointer to store the interval format type.
     *
     * \retval ::FWK_SUCCESS The operation was successful.
     * \return One of the standard framework error codes.
     */
    int (*get_update_intervals_info)(
        uint32_t *num_intervals,
        enum mod_telemetry_update_interval_formats *interval_format);

    /*!
     * \brief Get a specific telemetry update interval.
     *
     * \param[in] interval_index Index of the interval.
     * \param[out] sampling_rate Pointer to store the sampling rate in ms.
     *
     * \retval ::FWK_SUCCESS The operation was successful.
     * \return One of the standard framework error codes.
     */
    int (
        *get_update_interval)(uint32_t interval_index, uint32_t *sampling_rate);

    /*!
     * \brief Get the status of an enabled Data Event (DE).
     *
     * \param[in] de_handle Handle of the enabled DE.
     * \param[out] de_status Pointer to store the DE status.
     *
     * \retval ::FWK_SUCCESS The operation was successful.
     * \return One of the standard framework error codes.
     */
    int (*get_enabled_de_status)(
        telemetry_de_handle_st de_handle,
        struct mod_telemetry_de_status *de_status);

    /*!
     * \brief Disable a specific Data Event (DE).
     *
     * \param[in] de_handle Handle of the enabled DE to disable.
     *
     * \retval ::FWK_SUCCESS The operation was successful.
     * \return One of the standard framework error codes.
     */
    int (*disable_de)(telemetry_de_handle_st de_handle);

    /*!
     * \brief Enable a Data Event (DE) without an individual timestamp.
     *
     * \param[in] de_id ID of the DE to enable.
     * \param[out] de_handle Pointer to the constructed handle for the
     * requested DE.
     * \param[out] shmti_id SHMTI ID contianing the give Data Event.
     * \param[out] shmti_de_offset Byte offset from the start of the given
     *
     * \retval ::FWK_SUCCESS The operation was successful.
     * \return One of the standard framework error codes.
     */
    int (*enable_de_non_ts)(
        uint32_t de_id,
        uint32_t shmti_id,
        telemetry_de_handle_st *de_handle,
        uint32_t *shmti_de_offset);

    /*!
     * \brief Enable a Data Event (DE) with an individual timestamp.
     *
     * \param[in] de_id ID of the DE to enable.
     * \param[out] de_handle Pointer to the constructed handle for the
     * requested DE.
     * \param[out] shmti_id SHMTI ID contianing the give Data Event.
     * \param[out] shmti_de_offset Byte offset from the start of the given
     *
     * \retval ::FWK_SUCCESS The operation was successful.
     * \return One of the standard framework error codes.
     */
    int (*enable_de_ts)(
        uint32_t de_id,
        uint32_t shmti_id,
        telemetry_de_handle_st *de_handle,
        uint32_t *shmti_de_offset);

    /*!
     * \brief Disable telemetry functionality.
     *
     * \retval ::FWK_SUCCESS The operation was successful.
     * \return One of the standard framework error codes.
     */
    int (*telemetry_disable)(void);

    /*!
     * \brief Enable telemetry functionality.
     *
     * \retval ::FWK_SUCCESS The operation was successful.
     * \return One of the standard framework error codes.
     */
    int (*telemetry_enable)(void);

    /*!
     * \brief Set the telemetry data sampling rate.
     *
     * \param[in] sampling_rate Sampling rate in milliseconds.
     *
     * \retval ::FWK_SUCCESS The operation was successful.
     * \return One of the standard framework error codes.
     */
    int (*set_sampling_rate)(uint32_t sampling_rate);

    /*!
     * \brief Resets the telemetry infrastructure.
     *
     * \retval ::FWK_SUCCESS The operation was successful.
     * \return One of the standard framework error codes.
     */

    int (*telemetry_reset)(void);
};

/*!
 * \}
 */

/*!
 * \}
 */

#endif /* MOD_TELEMETRY_H */
