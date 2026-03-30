/*
 * Arm SCP/MCP Software
 * Copyright (c) 2024, Arm Limited and Contributors. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 * Description:
 *     Safety Island Support
 */

#ifndef MOD_SAFETY_ISLAND_PLATFORM_H
#define MOD_SAFETY_ISLAND_PLATFORM_H

#include <mod_power_domain.h>
#include <mod_transport.h>

#include <fwk_id.h>

#include <stdint.h>

/*!
 * \brief Indices of the interfaces exposed by the module.
 */
enum mod_safety_island_platform_api_idx {
    MOD_SAFETY_ISLAND_PLATFORM_API_IDX_BOOT_CLUSTER,

    /*! Number of exposed interfaces */
    MOD_SAFETY_ISLAND_PLATFORM_API_COUNT
};

/*!
 * \brief Safety Island cluster layout.
 */
struct safety_island_platform_cluster_layout {
    /*! Safety Island cluster ID */
    uint32_t id;
    /*! Safety Island cluster number of cores */
    uint32_t num_cores;
    /*! Safety Island cluster core offset */
    uint32_t core_offset;
};

/*!
 * \brief Safety Island cluster configuration data.
 */
struct safety_island_cluster_config {
    /*! Safety Island cluster transport channel identifier */
    const fwk_id_t transport_id;
    /* Safety Island cluster layout */
    const struct safety_island_platform_cluster_layout cluster_layout;
};

/*!
 * \brief Safety Island cluster context data.
 */
struct safety_island_cluster_ctx {
    /*! Safety Island cluster configuration data */
    struct safety_island_cluster_config *config;
    /* Transport API to send/respond to a message */
    struct mod_transport_firmware_api *transport_api;
    /* Power Domain restricted API pointer */
    struct mod_pd_restricted_api *pd_restricted_api;
};

#endif /* MOD_SAFETY_ISLAND_PLATFORM_H */
