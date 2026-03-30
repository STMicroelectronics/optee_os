/*
 * Arm SCP/MCP Software
 * Copyright (c) 2025, Arm Limited and Contributors. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 */

#include "internal/ni_710ae_discovery_drv.h"
#include "internal/ni_710ae_discovery_reg.h"

#include <fwk_string.h>

#include <stdbool.h>
#include <stdint.h>

#define INVALID_PTR ((uintptr_t)UINTPTR_MAX)

static inline bool ni710ae_type_is_domain(uint32_t type)
{
    switch (type) {
    case NI710AE_NODE_TYPE_CFGNI:
    case NI710AE_NODE_TYPE_VD:
    case NI710AE_NODE_TYPE_PD:
    case NI710AE_NODE_TYPE_CD:
    case NI710AE_NODE_TYPE_GCN:
        return true;
    default:
        return false;
    }
}

static inline bool ni710ae_type_is_component(uint32_t type)
{
    switch (type) {
    case NI710AE_NODE_TYPE_ASNI:
    case NI710AE_NODE_TYPE_AMNI:
    case NI710AE_NODE_TYPE_PMU:
    case NI710AE_NODE_TYPE_HSNI:
    case NI710AE_NODE_TYPE_HMNI:
    case NI710AE_NODE_TYPE_PMNI:
    case NI710AE_NODE_TYPE_FMU:
        return true;
    default:
        return false;
    }
}

static inline bool ni710ae_type_is_subfeature(uint32_t type)
{
    switch (type) {
    case NI710AE_NODE_TYPE_SUBFEATURE_APU:
    case NI710AE_NODE_TYPE_SUBFEATURE_SAM:
    case NI710AE_NODE_TYPE_SUBFEATURE_FCU:
    case NI710AE_NODE_TYPE_SUBFEATURE_IDM:
    case NI710AE_NODE_TYPE_SUBFEATURE_RAS:
        return true;
    default:
        return false;
    }
}

static struct ni710ae_discovery_tree_t *allocate_node(
    struct ni710ae_discovery_tree_t *memory_pool,
    uint16_t *memory_index,
    uint16_t max_node_count)
{
    if (*memory_index >= max_node_count) {
        return NULL; // Memory exhausted
    }

    struct ni710ae_discovery_tree_t *node = &memory_pool[*memory_index];
    (*memory_index)++; // Move to next available slot
    memset(node, 0, sizeof(*node));

    return node;
}

/*
 * NI710AE Discovery
 */
int ni710ae_discovery(
    struct ni710ae_discovery_tree_t *cfg_node,
    const uintptr_t periphbase_addr,
    struct ni710ae_discovery_tree_t *memory_pool,
    uint16_t *memory_index,
    uint16_t max_node_count)
{
    if ((cfg_node == NULL) || (memory_pool == NULL) || (memory_index == NULL)) {
        return FWK_E_PARAM;
    }

    uint16_t type, id;
    uint32_t address, child_count, c_idx = 0;
    uintptr_t hdr_base = periphbase_addr + cfg_node->address;

    struct ni710ae_discovery_tree_t *node;
    struct ni710ae_discovery_tree_t *sibling = NULL;

    int err;

    /* Fetch number of children */
    if (ni710ae_type_is_domain(cfg_node->type)) {
        child_count =
            ((struct ni710ae_domain_cfg_hdr *)(hdr_base))->child_node_info;
    } else if (ni710ae_type_is_component(cfg_node->type)) {
        if (cfg_node->type == NI710AE_NODE_TYPE_PMU) {
            /* Skipping because PMU doesn't have children */
            return FWK_SUCCESS;
        }
        child_count =
            ((struct ni710ae_component_cfg_hdr *)(hdr_base))->num_sub_features;
    } else if (ni710ae_type_is_subfeature(cfg_node->type)) {
        return FWK_SUCCESS;
    } else {
        return FWK_E_PARAM;
    }

    for (; c_idx < child_count; ++c_idx) {
        if (ni710ae_type_is_domain(cfg_node->type)) {
            address =
                ((struct ni710ae_domain_cfg_hdr *)hdr_base)->x_pointers[c_idx];
            struct ni710ae_domain_cfg_hdr *child_hdr =
                (struct ni710ae_domain_cfg_hdr *)(periphbase_addr + address);
            type = child_hdr->node_type & 0xFFFF;
            id = child_hdr->node_type >> 16;
        } else {
            struct ni710ae_component_cfg_hdr *child_hdr =
                (struct ni710ae_component_cfg_hdr
                     *)(periphbase_addr + cfg_node->address);
            address = child_hdr->sub_feature[c_idx].pointer;

            if (child_hdr->sub_feature[c_idx].type >=
                NI710AE_NODE_TYPE_SUBFEATURE_MAX_LIMIT) {
                return FWK_E_DATA;
            }

            type = child_hdr->sub_feature[c_idx].type;
            id = cfg_node->id;
        }
        node = allocate_node(memory_pool, memory_index, max_node_count);

        if (node == NULL) {
            return FWK_E_NOMEM;
        }

        node->type = type;
        node->id = id;
        node->address = address;
        node->sibling = sibling;

        cfg_node->child = node;
        cfg_node->children++;
        sibling = node;

        err = ni710ae_discovery(
            node, periphbase_addr, memory_pool, memory_index, max_node_count);
        if (err != FWK_SUCCESS) {
            return err;
        }
    }

    return FWK_SUCCESS;
}

uintptr_t ni710ae_fetch_offset_address(
    struct ni710ae_discovery_tree_t *root,
    uint16_t component_type,
    uint16_t component_id,
    uint16_t sub_feature_type)
{
    struct ni710ae_discovery_tree_t *node;
    uintptr_t ret_addr;

    if (root == NULL) {
        return INVALID_PTR;
    }

    if ((root->id == component_id) && (root->type == component_type)) {
        node = root->child;
        while (node != NULL) {
            if (sub_feature_type == node->type) {
                return node->address;
            }
            node = node->sibling;
        }
    }

    ret_addr = ni710ae_fetch_offset_address(
        root->child, component_type, component_id, sub_feature_type);
    if (ret_addr != INVALID_PTR) {
        return ret_addr;
    }

    return ni710ae_fetch_offset_address(
        root->sibling, component_type, component_id, sub_feature_type);
}
