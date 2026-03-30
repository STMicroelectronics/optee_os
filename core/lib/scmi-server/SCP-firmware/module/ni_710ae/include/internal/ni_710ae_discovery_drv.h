/*
 * Arm SCP/MCP Software
 * Copyright (c) 2025, Arm Limited and Contributors. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 */

#ifndef NI_710AE_DISCOVERY_DRV_H
#define NI_710AE_DISCOVERY_DRV_H

#include "ni_710ae_drv.h"

#include <fwk_status.h>

#include <stdint.h>

/*!
 * \brief NI-710AE Discovery tree structure
 *
 * The discovery tree uses a parent–child–sibling representation:
 * child points to the first child node and sibling points to the
 * next node at the same hierarchy level.
 *
 * Example hierarchy list and its discovery tree:
 *
 * GCN Root (domain)
 *  └─CD (domain)
 *     └─ASNI0 (component)
 *     └─ASNI1 (component)
 *     |   └─APU (subfeature)
 *     └─AMNI0 (component)
 *         └─APU (subfeature)
 *
 * Discovery tree;
 * ------------------------
 *   Root Global Configuration Node - GCN (domain)
 *   └─ sibling → NULL
 *   └─ child → CD0 (domain)
 *          └─ sibling → NULL
 *          └─ child → ASNI0 (component)
 *             ├─ child → NULL
 *             └─ sibling → ASNI1 (component)
 *                        ├─ child → APU (subfeature)
 *                        └─ sibling → AMNI0 (component)
 *                                   ├─ child → APU (subfeature)
 *                                   └─ sibling → NULL
 */
struct ni710ae_discovery_tree_t {
    /*! Node type code. */
    uint16_t type;
    /*! Node instance ID within its parent. */
    uint16_t id;
    /*! Register address. */
    uint32_t address;
    /*! Number of child nodes. */
    uint32_t children;
    /*! First child node. */
    struct ni710ae_discovery_tree_t *child;
    /*! Next node at the same hierarchy level. */
    struct ni710ae_discovery_tree_t *sibling;
};

/*!
 * \brief NI-710AE Discovery prune node structure
 */
struct ni710ae_prune_node_t {
    /*! Node type code. */
    uint16_t type;
    /*! Node instance ID. */
    uint16_t id;
};

/*!
 * \brief Executes NI-710AE discovery flow
 *
 * \param[in] cfg_node          NI-710AE Discovery struct
 * \param[in] periphbase_addr   NI-710AE base address, same as CFGNI0 address
 * \param[in] memory_pool       Memory pool for dynamic node allocations
 * \param[in] memory_index      Memory index to keep track of memory_pool
 * \param[in] max_node_count    Max number of nodes for the provided memory_pool
 *
 * \retval ::FWK_SUCCESS Operation succeeded.
 * \retval ::FWK_E_PARAM One or more parameters were invalid.
 */
int ni710ae_discovery(
    struct ni710ae_discovery_tree_t *cfg_node,
    const uintptr_t periphbase_addr,
    struct ni710ae_discovery_tree_t *memory_pool,
    uint16_t *memory_index,
    uint16_t max_node_count);

/*!
 * \brief Fetches sub-feature base address based on the parent component ID and
 *        type.
 *
 * \param[in] root              NI-710AE Discovery struct root node
 * \param[in] component_type    Specify the parent component ID of the
 *                              sub-feature
 * \param[in] component_id      Specify the parent component type of
 *                              the sub-feature
 * \param[in] sub_feature_type  Specify the type of the sub-feature
 *
 * \return Returns offset address
 */
uintptr_t ni710ae_fetch_offset_address(
    struct ni710ae_discovery_tree_t *root,
    uint16_t component_type,
    uint16_t component_id,
    uint16_t sub_feature_type);

#endif /* NI_710AE_DISCOVERY_DRV_H */
