/*
 * Arm SCP/MCP Software
 * Copyright (c) 2025, Arm Limited and Contributors. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 */

#ifndef NI_710AE_DISCOVERY_REG_H
#define NI_710AE_DISCOVERY_REG_H

#include <fwk_macros.h>

#include <stdint.h>

/* NI-710AE Domain top registers. These are common for all domains */
struct ni710ae_domain_cfg_hdr {
    FWK_R uint32_t node_type;
    FWK_R uint32_t child_node_info;
    FWK_R uint32_t x_pointers[];
};

/* NI-710AE Sub-feature register set. Found in component domain */
struct ni710ae_sub_feature_cfg_attr {
    FWK_R uint32_t type;
    FWK_R uint32_t pointer;
};

/* NI-710AE Component top registers. These are common for all components */
struct ni710ae_component_cfg_hdr {
    FWK_R uint32_t node_type;
    const uint32_t reserved_0[8];
    FWK_R uint32_t num_sub_features;
    struct ni710ae_sub_feature_cfg_attr sub_feature[];
};

#endif /* NI_710AE_DISCOVERY_REG_H */
