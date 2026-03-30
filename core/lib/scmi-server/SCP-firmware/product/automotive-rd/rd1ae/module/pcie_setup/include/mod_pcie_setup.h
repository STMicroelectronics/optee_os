/*
 * Arm SCP/MCP Software
 * Copyright (c) 2024, Arm Limited and Contributors. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 * Description:
 *     Module for configuring PCIe rootports and its memory map.
 */

#ifndef MOD_PCIE_SETUP_H
#define MOD_PCIE_SETUP_H

#include <fwk_id.h>
#include <fwk_module_idx.h>

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

/*!
 * \brief PCIe setup notification indices.
 */
enum mod_pcie_setup_notification_idx {
    /*! PCIe initialisation completed. */
    MOD_PCIE_SETUP_NOTIFICATION_IDX_INITIALISED,
    /*! Number of defined notification. */
    MOD_PCIE_SETUP_NOTIFICATION_IDX_COUNT
};

/*!
 * \brief Defines base address and size of carveout.
 */

struct mod_pcie_setup_carveout {
    /*! Start of the carveout */
    uint64_t start;
    /*! Size of the carveout */
    uint64_t size;
};

/*!
 * \brief Defines regions for ECAM, MMIOL, MMIOH and Bus.
 */
struct mod_pcie_setup_mmap {
    struct mod_pcie_setup_carveout ecam;
    struct mod_pcie_setup_carveout mmiol;
    struct mod_pcie_setup_carveout mmioh;
    struct mod_pcie_setup_carveout bus;
    uint64_t interrupt_id;
};

/*!
 * \brief Placeholder to put ECAM, MMIOL, MMIOH and Bus number info for each RP.
 */
struct mod_pcie_setup_block_mmap {
    uint64_t id;
    uint64_t segment;
    uint64_t translation;
    uint64_t smmu_base;
    uint64_t ep_count;
    struct mod_pcie_setup_mmap ep[];
};

/*!
 * \brief Placeholder to put information of all the block present on the
 *        platform.
 */
struct mod_pcie_setup_block_mmap_list {
    uint64_t block_count;
    uint64_t table_size;
    struct mod_pcie_setup_block_mmap blocks[];
};

/*!
 * \brief configuration for a single end point and memory map for non io macro.
 */
struct mod_pcie_setup_ep_config {
    /*! Valid flag to indicate if the mapping is valid to program */
    bool valid;
    /*! Allow non-secure access
     *
     * By default only secure accesses are allowed. Set this to true to allow
     * non-secure access as well.
     */
    bool allow_ns_access;
    /*! ID of the Root port node. */
    uint8_t rp_node_id;
};

/*!
 * \brief Configuration information used for programming IO virtualisation
 *        block.
 */
struct mod_pcie_setup_config {
    /*! NCI GVP base */
    uint64_t reg_base;
    /*! ID of the node in the CMN mesh configuration */
    unsigned int cmn_node_id;
    /*!* ID of the source node in NCI */
    uint8_t nci_source_node_id;
    /*!* ID of the Io Virtualization block. */
    uint8_t block_id;
    /*! Identifier of the SDS structure to place PCIE mmap info. */
    uint32_t sds_struct_id;
    /*! SMMU base address for the block. */
    uint64_t smmu_base;
    /*! End point config for RPs */
    struct mod_pcie_setup_ep_config *ep_config;
};

/*!
 * \brief Platform notification source and notification id
 *
 * \details On platforms that require platform configuration to access the SDS
 *          memory regions, the platform notification can be subscribed. This is
 *          optional for a platform and if provided as module configuration
 *          data, the configuration will be done only after this notification is
 *          processed.
 */
struct mod_pcie_setup_platform_notification {
    /*! Identifier of the notification id */
    const fwk_id_t notification_id;

    /*! Identifier of the module sending the notification */
    const fwk_id_t source_id;
};

/*!
 * \brief Module configuration data used for per chip resource allocation.
 */
struct mod_pcie_setup_resource_info {
    /*! ID of concrete implementation of CMN interface. */
    fwk_id_t cmn_id;
    /*! ID of concrete implementation of IO Block interface. */
    fwk_id_t io_block_id;
    /*! ID of concrete implementation of ATU MMIO. */
    fwk_id_t atu_mmio_id;
    /*! ID of CMN map_io_region api. */
    fwk_id_t cmn_api_id;
    /*! ID of IO Block io_block_setup api. */
    fwk_id_t io_block_api_id;
    /*! ID of IO Block ATU MMIO rw api. */
    fwk_id_t atu_mmio_api_id;
    /*! Offset added to the address. */
    uint64_t translation;
    /*! Bank for allocating carveouts for ECAM, MMIOL and MMIOH. */
    struct mod_pcie_setup_mmap mmap;
    /*! Number of endpoints in each IO Block. */
    uint8_t ep_count;
    /*! Platform notification source and notification id */
    struct mod_pcie_setup_platform_notification plat_notification;
    /*! Structure ID for PCIe configuration info in SDS table. */
    uint32_t sds_struct_id;
    /*! List of the interrupt IDs for the end points. */
    uint64_t *ep_interrupt_ids;
};

/*!
 * \brief Module configuration.
 */
struct mod_pcie_setup_module_config {
    /*! Number of resource information configuration. One per chip. */
    size_t resource_info_count;
    /*!
     * Resource configuration common for all the IO virtualsation blocks in a
     * chip.
     */
    struct mod_pcie_setup_resource_info *resource_info;
};

/*!
 * \brief Identifiers for the ::MOD_PCIE_SETUP_NOTIFICATION_IDX_INITIALISED
 *        notification.
 */
static const fwk_id_t mod_pcie_setup_notification_initialised =
    FWK_ID_NOTIFICATION_INIT(
        FWK_MODULE_IDX_PCIE_SETUP,
        MOD_PCIE_SETUP_NOTIFICATION_IDX_INITIALISED);

#endif /* MOD_PCIE_SETUP_H */
