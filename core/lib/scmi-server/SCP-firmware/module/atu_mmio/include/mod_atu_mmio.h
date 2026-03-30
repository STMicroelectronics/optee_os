/*
 * Arm SCP/MCP Software
 * Copyright (c) 2024-2025, Arm Limited and Contributors. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 * Description:
 *     ATU MMIO module acts as an interface for accessing addresses that are
 *     outside the addressable memory map of the processor. It does not need
 *     permanent mapping in the ATU. It works by reserving a carveout used for
 *     mapping it to any region outside the processor's address space. Memory
 *     space and exposing API for byte, halfword, word and doubleword Read/Write
 *     APIs. These APIs maps a given address to the reserved carveout window and
 *     then performs the Read/Write operation. If the subsequent address falls
 *     within the window, then the address mapping step is skipped and performs
 *     the Read/Write operation directly.
 */

#ifndef MOD_ATU_MMIO_H
#define MOD_ATU_MMIO_H

#include <fwk_id.h>

#include <stddef.h>
#include <stdint.h>

/*!
 * \addtogroup GroupModules Modules
 * \{
 */

/*!
 * \ingroup GroupModules
 * \defgroup GroupModuleAtuMmio ATU MMIO
 *
 * \brief ATU MMIO module.
 *
 * \details This module implements Address Remapper.
 * \{
 */

/*!
 * \brief Module API indices.
 */
enum mod_atu_mmio_api_idx {
    /*! Interface to byte, halfword, word and double word read write APIs. */
    MOD_ATU_MMIO_API_IDX_MEM_RW,
    /*! Total API count. */
    MOD_ATU_MMIO_API_IDX_MAX,
};

/*!
 * \brief ATU MMIO module configuration data.
 */
struct mod_atu_mmio_config {
    /*! Base address of the carveout used for remapping. */
    uintptr_t window_address;
    /*! Size of the carveout. */
    uintptr_t map_size;
    /*! ATU module ID. */
    fwk_id_t atu_id;
    /*! API in the ATU module used for remapping. */
    fwk_id_t atu_api_id;
};

/*!
 * \}
 */

/*!
 * \}
 */
#endif /* MOD_ATU_MMIO_H */
