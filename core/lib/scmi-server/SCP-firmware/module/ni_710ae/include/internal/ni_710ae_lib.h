/*
 * Arm SCP/MCP Software
 * Copyright (c) 2025, Arm Limited and Contributors. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 */

#ifndef NI_710AE_LIB_H
#define NI_710AE_LIB_H

#include "ni_710ae_apu_drv.h"
#include "ni_710ae_discovery_drv.h"

#include <fwk_status.h>

/*!
 * \brief APU subregion configuration.
 *
 * Describes a single address subregion and its protection attributes for a
 * requester/completer APU.
 */
struct ni_710ae_apu_subregion_configs {
    /*! Start address of the subregion. */
    uint64_t base_addr;
    /*! End address of the subregion. */
    uint64_t end_addr;
    /*! BR type. */
    enum ni710ae_apu_br_type_t br;
    /*! Permissions bitmask. */
    uint32_t perms;
    /*! Region index. */
    uint8_t region;
};

/*!
 * \brief APU configuration for a single NI-710AE component.
 *
 * Identifies the component and lists the subregions to be programmed.
 */
struct ni_710ae_component_apu_config {
    /*! Component type code. */
    uint16_t component_type;
    /*! Component instance ID. */
    uint16_t component_id;
    /*! Subregions array. */
    const struct ni_710ae_apu_subregion_configs *regions;
    /*! Number of entries in regions. */
    uint16_t apu_subregion_count;
};

/*!
 * \brief Programs NI-710AE APUs with the given config
 *
 * \param[in] config                NI-710AE APU Configs Array
 * \param[in] apu_config_count      NI-710AE APU Config Count
 * \param[in] dev_periphbase_addr   NI-710AE Base Address for Configuration
 * \param[in] discovery_tree        NI-710AE Discovery tree
 *
 * \retval ::FWK_SUCCESS Operation succeeded.
 * \retval ::FWK_E_PARAM One or more parameters were invalid.
 */
int program_ni_710ae_apu(
    const struct ni_710ae_component_apu_config *config,
    uint16_t apu_config_count,
    uintptr_t dev_periphbase_addr,
    struct ni710ae_discovery_tree_t *discovery_tree);

#endif /* NI_710AE_LIB_H */
