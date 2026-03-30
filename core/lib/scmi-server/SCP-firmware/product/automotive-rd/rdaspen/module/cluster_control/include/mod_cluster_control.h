/*
 * Arm SCP/MCP Software
 * Copyright (c) 2025, Arm Limited and Contributors. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef MOD_CLUSTER_CONTROL_H
#define MOD_CLUSTER_CONTROL_H

#include <fwk_id.h>

#include <stdint.h>

/*!
 * \addtogroup GroupPLATFORMModule PLATFORM Product Modules
 * \{
 */

/*!
 * \defgroup GroupClusterControl RD-Aspen Cluster Control driver
 *
 * \details A driver to initialize the RD-Aspen cluster control registers.
 *
 * \{
 */

/*!
 * \brief Platform notification source and notification id
 *
 */
struct mod_cluster_control_platform_notification {
    /*! Identifier of the notification id */
    const fwk_id_t notification_id;

    /*! Identifier of the module sending the notification */
    const fwk_id_t source_id;
};

/*!
 * \brief Cluster control configuration.
 */
struct mod_cluster_control_config {
    /*! Array of cluster control regions to program */
    const uintptr_t *regions;

    /*! The number of cluster control regions */
    unsigned int region_count;

    /*! The value to write to the RVBAR register */
    uint64_t rvbar;

    /*! Platform notification source and notification id (optional) */
    struct mod_cluster_control_platform_notification platform_notification;
};

/*!
 * \}
 */

/*!
 * \}
 */

#endif /* MOD_CLUSTER_CONTROL_H */
