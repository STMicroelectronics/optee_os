/*
 * Arm SCP/MCP Software
 * Copyright (c) 2025, Arm Limited and Contributors. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 */

#ifndef MOD_NI_710AE_H
#define MOD_NI_710AE_H

#include "ni_710ae_lib.h"

#include <fwk_id.h>

#include <stdint.h>

#define MOD_NI_710AE_MAX_APU_SUBREGION_COUNT (32U)
#define MOD_NI_710AE_MAX_NUMBER_OF_NODES     (255U)
/*!
 * \addtogroup GroupModules Modules
 * \{
 */

/*!
 * \defgroup GroupNI_710AE NI_710AE
 * \brief NI_710AE module for managing NCI discovery and APU configurations.
 * \{
 */

/*
 *                      +------------------------------------+
 *                      |              NI_710AE              |
 *                      |                                    |
 *                      |   +-----+               +-----+    |
 *  AXI & ACE-Lite      |   | ASNI|               |AMNI |    |   AXI & ACE-Lite
 *  Completer           |   +-----+               +-----+    |    Requester
 *  Interfaces -------->|                                    |<--- Interfaces
 *                      |                                    |
 *  AHB Completer       |   +-----+               +-----+    |   AHB Requester
 *  Interfaces -------->|   | HSNI|               |HMNI |    |<--- Interfaces
 *                      |   +-----+               +-----+    |
 *                      |                                    |
 *                      |                                    |
 *                      |                                    |
 *                      |                                    |
 *                      |                                    |
 *                      |                          +-----+   |
 *                      |                          |PMNI |   |   APB Requester
 *                      |                          +-----+   |<--- Interfaces
 *                      |                                    |
 *                      +------------------------------------+
 */

/*!
 * \brief Configuration structure for an NI-710AE element.
 *
 * Holds the base address and optional APU access protection configuration,
 * as well as discovery tree sizing information.
 */
struct mod_ni_710ae_element_config {
    /*! NI-710AE PERIPHBASE/CFGNI0 base address. */
    const uintptr_t periphbase_addr;
    /*! APU configs array. */
    const struct ni_710ae_component_apu_config *apu_configs;
    /*! Number of entries in apu_configs. */
    const uint16_t apu_config_count;
    /*! Max nodes to allocate for discovery. */
    const uint16_t max_number_of_nodes;
};

/*!
 * \}
 */

/*!
 * \}
 */

#endif /* MOD_NI_710AE_H */
