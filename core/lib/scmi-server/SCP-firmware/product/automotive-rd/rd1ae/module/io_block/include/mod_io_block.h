/*
 * Arm SCP/MCP Software
 * Copyright (c) 2024, Arm Limited and Contributors. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 * Description:
 *     IO Block driver module to define IO block configuration..
 */

#ifndef MOD_IO_BLOCK_H
#define MOD_IO_BLOCK_H

#include <interface_io_block.h>

#include <fwk_id.h>

#include <stddef.h>
#include <stdint.h>

/*!
 * \addtogroup GroupModules Modules
 * \{
 */

/*!
 * \ingroup GroupModules Modules
 * \defgroup GroupModuleIOBlock IO Block
 *
 * \brief IO Block module.
 *
 * \details This module implements IO Block layout for the platform.
 * \{
 */

/*!
 * \brief Module API indices
 */
enum mod_io_block_api_idx {
    /*! Interface to configure a region in the target IO block */
    MOD_IO_BLOCK_API_IDX_MAP_REGION,
    /*! Total API count */
    MOD_IO_BLOCK_API_IDX_MAX,
};

/*!
 * \brief Info of each IO block and regions to be programmed in it.
 */
struct mod_io_block_element_config {
    /*! Number of mapping for the IO Block */
    uint32_t region_mem_maps_count;
    /*! List of the mapping to be done for the IO Block */
    struct interface_io_block_setup_mmap *region_mem_maps;
};

/*!
 * \brief Used for capturing notification ID and source ID of the platform
 *        module.
 */
struct mod_io_block_platform_notification {
    /*! Identifier of the notification id */
    const fwk_id_t notification_id;
    /*! Identifier of the module sending the notification */
    const fwk_id_t source_id;
};

/*!
 * \brief IO Block module configuration data
 */
struct mod_io_block_config {
    /*! NCI module ID */
    fwk_id_t nci_id;
    /*! NCI module address mapping API ID */
    fwk_id_t nci_api_id;
    /*! ATU module ID */
    fwk_id_t atu_id;
    /*! ATU module address mapping API ID */
    fwk_id_t atu_api_id;
    /*! Platform notification and source ID */
    struct mod_io_block_platform_notification plat_notification;
    /*!
     * Logical address used to map 64 bit IO Block register address to a 32 bit
     * window
     */
    uintptr_t logical_region_base;
    /*! size of the region */
    size_t logical_region_size;
};

/*!
 * \}
 */

/*!
 * \}
 */
#endif /* MOD_IO_BLOCK_H */
