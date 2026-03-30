/*
 * Arm SCP/MCP Software
 * Copyright (c) 2025, Arm Limited and Contributors. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef FMU_IMPL_API_H
#define FMU_IMPL_API_H

#include <mod_fmu.h>

#include <fwk_id.h>

#include <stdbool.h>
#include <stdint.h>

#define MOD_NAME "[FMU] "

#define LSB_GET(value) ((value) & -(value))

struct mod_fmu_impl_api {
    /*! Consume the module configuration */
    int (*configure)(const struct mod_fmu_config *config);

    /*! Bind to other modules */
    int (*bind)(fwk_id_t id);

    /*! Discover the next reported fault (required)
     *
     * @param config The selected device configuration
     * @param node_idx Index of the first node that currently has a valid fault
     * @return false if no fault record is pending in this device, else true
     */
    bool (*fault_peek)(
        const struct mod_fmu_dev_config *config,
        unsigned int *node_idx);

    /*! Clear a specific fault record and return its details (required)
     *
     * @param config The selected device configuration
     * @param fault Populated with node_idx and SMID of the cleared record
     * @param node_idx Node index (previously obtained via fault_peek())
     * @param fault_tracked Points to false only on first call to this function,
     *        in order to store fault of the most-childish device
     */
    void (*fault_ack)(
        const struct mod_fmu_dev_config *config,
        struct mod_fmu_fault *fault,
        unsigned int node_idx,
        bool *fault_tracked);

    /*! Inject a fault */
    int (*inject)(
        const struct mod_fmu_dev_config *config,
        const struct mod_fmu_fault *fault);

    /*! Get the enabled status for the given id and node_id */
    int (*get_enabled)(
        const struct mod_fmu_dev_config *config,
        const struct mod_fmu_fault *fault,
        bool *enabled);

    /*! Enable or disable fault reporting for the given id and node_id */
    int (*set_enabled)(
        const struct mod_fmu_dev_config *config,
        const struct mod_fmu_fault *fault,
        bool enabled);

    /*! Get the current fault count for the given id and node_id */
    int (*get_count)(
        const struct mod_fmu_dev_config *config,
        uint16_t node_id,
        uint8_t *count);

    /*! Set the current fault count for the given id and node_id */
    int (*set_count)(
        const struct mod_fmu_dev_config *config,
        uint16_t node_id,
        uint8_t count);

    /*! Get the fault upgrade threshold for the given id and node_id */
    int (*get_threshold)(
        const struct mod_fmu_dev_config *config,
        uint16_t node_id,
        uint8_t *threshold);

    /*! Set the fault upgrade threshold for the given id and node_id */
    int (*set_threshold)(
        const struct mod_fmu_dev_config *config,
        uint16_t node_id,
        uint8_t threshold);

    /* Get the status of the fault upgrade feature */
    int (*get_upgrade_enabled)(
        const struct mod_fmu_dev_config *config,
        uint16_t node_id,
        bool *enabled);

    /*! Enable or disable whether a fault is upgraded to critical when the
     * threshold is reached */
    int (*set_upgrade_enabled)(
        const struct mod_fmu_dev_config *config,
        uint16_t node_id,
        bool enabled);

    /*! Configure the criticality of a safety mechanism */
    int (*set_critical)(
        const struct mod_fmu_dev_config *config,
        const struct mod_fmu_fault *fault,
        bool critical);
};

#endif
