/*
 * Arm SCP/MCP Software
 * Copyright (c) 2025, Arm Limited and Contributors. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 * Description:
 *     Telemetry and module contexts.
 */

#ifndef TELEMETRY_H
#define TELEMETRY_H

#include <mod_telemetry.h>
#include <mod_timer.h>

/*!
 * \brief SHMTI (Shared Memory Telemetry Interface) context structure.
 *
 * Holds information regarding SHMTI region, allocation tracking, and
 * metadata.
 */
struct telemetry_shmti_context {
    /*! Flag indicating SHMTI creation status */
    bool shmti_created;
    /*! Pointer to SHMTI metadata */
    const struct mod_telemetry_shmti_info *shmti_info;
    /*! Bitmap tracking allocated blocks */
    uint8_t *allocation_map;
    /*! Length of the bitmap */
    uint32_t bitmap_len;
    /*! Address for the SHMTI end sequence */
    uintptr_t end_seq_addr;
};

/*!
 * \brief Data Event (DE) descriptor and its runtime status.
 *
 * This structure holds a Data Event descriptor along with its status.
 */
struct telemetry_de_status {
    /*! Timestamp mode for this DE. */
    bool ts_enabled;
    /*! SHMTI context DE resides in. */
    struct telemetry_shmti_context *curr_shmti_ctx;
    /*! Offset of the allocated in SHMTI */
    uint32_t shmti_de_offset;
};

/*!
 * \brief Telemetry source (driver) context structure.
 *
 * This structure manages telemetry sources, their associated APIs, and
 * allocated data events.
 */
struct telemetry_source_context {
    /*! Source driver config. */
    const struct mod_telemetry_source_config *config;
    /*! Source Index */
    uint32_t index;
    /*! API for telemetry source */
    const struct mod_telemetry_driver_api *api;
    /*! Number of Data Events (DEs) */
    uint32_t num_de;
    /*! List of DEs */
    const struct mod_telemetry_de_desc *de_list;
    /*! List of Data Event */
    struct telemetry_de_status *de_status_list;
    /*! Flag to indicate if data is updated */
    bool is_data_updated;
    /*! Count of enabled DEs */
    uint32_t de_enabled_count;
    /*! De Data array. */
    struct mod_telemetry_de_data *de_data_list;
};

/*!
 * \brief Global telemetry module context.
 *
 * Stores state, telemetry sources, and SHMTI management structures.
 */
struct mod_telemetry_context {
    /*! Number of sources */
    uint32_t num_sources;
    /*! Telemetry sources */
    struct telemetry_source_context *source_ctx_table;
    /*! Flag indicating telemetry is enabled */
    bool telemetry_enabled;
    /*! Telemetry sampling rate */
    uint32_t current_sampling_rate_msecs;
    /*! Total number of Data Events */
    uint32_t total_de_count;
    /*! Total enabled Data Events */
    uint32_t total_de_enabled_count;
    /*! Telemetry configuration */
    const struct mod_telemetry_config *config;
    /*! Number of SHMTIs. */
    uint32_t shmti_count;
    /*! SHMTI context */
    struct telemetry_shmti_context *shmti_ctx_table;
    /*! Internal flag to indicate if telemetry updates are ongoing. */
    bool is_telemetry_update_ongoing;
    /*! Alarm API for periodic sampling intervals */
    const struct mod_timer_alarm_api *alarm_api;
};

/* SHMTI APIs */
/*!
 * \brief Creates an SHMTI (Shared Memory Telemetry Interface) context.
 *
 * \details This function initializes the SHMTI memory structure, including
 *          setting up its allocation bitmap and marking necessary metadata
 *          areas as reserved.
 *
 * \param[in,out] shmti_ctx Pointer to the SHMTI context structure.
 *
 * \retval FWK_SUCCESS if SHMTI creation is successful.
 * \retval FWK_E_PARAM if the input context is NULL.
 * \retval FWK_E_NOMEM if bitmap allocation fails.
 */
int shmti_create(struct telemetry_shmti_context *shmti_ctx);

/*!
 * \brief Allocates a memory pool in an SHMTI region.
 *
 * \details This function searches for a contiguous free block of memory
 *          within the SHMTI allocation bitmap and marks it as used if found.
 *
 * \param[in,out] shmti_ctx Pointer to the SHMTI context structure.
 * \param[in] size The size of the memory block to allocate.
 * \param[out] addr Pointer to store the allocated memory address.
 *
 * \retval FWK_SUCCESS if the allocation was successful.
 * \retval FWK_E_STATE if the SHMTI region is not initialized.
 * \retval FWK_E_NOMEM if there is no sufficient free space.
 */
int shmti_alloc_pool(
    struct telemetry_shmti_context *shmti_ctx,
    size_t bytes_to_be_allocated,
    uint32_t *offset);

/*!
 * \brief Frees an allocated memory block in an SHMTI region.
 *
 * \details This function marks a previously allocated memory block as free
 *          in the SHMTI allocation bitmap, making it available for future
 * allocations.
 *
 * \param[in,out] shmti_ctx Pointer to the SHMTI context structure.
 * \param[in] size The size of the memory block to free.
 * \param[in] addr The starting address of the allocated block.
 *
 * \retval FWK_SUCCESS if the memory block is successfully freed.
 * \retval FWK_E_PARAM if the provided SHMTI ID or address is invalid.
 */

int shmti_free_pool(
    struct telemetry_shmti_context *shmti_ctx,
    size_t bytes_allocated,
    uint32_t offset);

/*!
 * \brief Increments the sequence number for all SHMTI regions.
 *
 *
 * \details This function updates the start and end sequence numbers for
 *          all SHMTI (Shared Memory Telemetry Interface) regions. The sequence
 *          numbers help track telemetry data updates, ensuring consistency
 *          between the start and end markers.
 *
 * \param[in] shmti_ctx Pointer to the SHMTI context structure.
 *
 */
int shmti_seq_inc(const struct telemetry_shmti_context *shmti_ctx);

#endif /* !TELEMETRY_H */
