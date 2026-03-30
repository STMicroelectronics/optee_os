/*
 * Arm SCP/MCP Software
 * Copyright (c) 2024, Arm Limited and Contributors. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef MOD_ARMV8R_MPU_H
#define MOD_ARMV8R_MPU_H

#include <stddef.h>
#include <stdint.h>

/*!
 * \ingroup GroupModules
 * \addtogroup GroupMPUARMv8R MPU (ARMv8-R64)
 * \{
 */

/*!
 * \brief MPU_MAIR registers indices.
 */
enum mod_armv8m_mpu_attr_id {
    MPU_ATTR_0,
    MPU_ATTR_1,
    MPU_ATTR_2,
    MPU_ATTR_3,
    MPU_ATTR_4,
    MPU_ATTR_5,
    MPU_ATTR_6,
    MPU_ATTR_7,
    MPU_MAX_ATTR_COUNT,
};

/*!
 * \brief Armv8R64 region configuration
 */
struct mod_armv8r_mpu_region {
    /*!
     * \brief Provides access to the base address of the MPU region
     */
    uint64_t prbar;

    /*!
     * \brief Provides access to the limit address of the MPU region
     */
    uint64_t prlar;
};

/*!
 * \brief Module configuration.
 */
struct mod_armv8r_mpu_config {
    /*!
     * \brief Number of MPU attributes.
     */
    uint8_t attributes_count;

    /*!
     * \brief Pointer to array of MPU attributes.
     */
    const uint8_t *attributes;

    /*!
     * \brief Number of MPU regions.
     */
    uint32_t region_count;

    /*!
     * \brief Pointer to array of MPU regions.
     */
    const struct mod_armv8r_mpu_region *regions;
};

/*!
 * \}
 */

#endif /* MOD_ARMV8R_MPU_H */
