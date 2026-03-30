/*
 * Arm SCP/MCP Software
 * Copyright (c) 2025, Arm Limited and Contributors. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 */
#include <mod_ni_710ae.h>

#include <fwk_id.h>
#include <fwk_log.h>
#include <fwk_macros.h>
#include <fwk_mm.h>
#include <fwk_module.h>
#include <fwk_status.h>

#include <stddef.h>

struct mod_ni_710ae_element_ctx {
    const char *element_name;
    /* Points to the configuration of the element. */
    struct mod_ni_710ae_element_config *config;

    /* Discovery Tree root node (filled during discovery process)*/
    struct ni710ae_discovery_tree_t discovery_tree;

    /* Memory pool for NCI tree discovery process. */
    struct ni710ae_discovery_tree_t *discovery_mem_pool;
    uint16_t discovery_mem_pool_index;
};

struct mod_ni_710ae_ctx {
    /* List of the element's context. */
    struct mod_ni_710ae_element_ctx *element_ctx;
    /* Number of elements. */
    unsigned int element_count;
};

static struct mod_ni_710ae_ctx ni_710ae_ctx;

static int mod_ni_710ae_init(
    fwk_id_t module_id,
    unsigned int element_count,
    const void *unused)
{
    if (element_count == 0U) {
        /* No element to configure */
        return FWK_E_PARAM;
    }

    ni_710ae_ctx.element_ctx =
        fwk_mm_calloc(element_count, sizeof(*ni_710ae_ctx.element_ctx));

    if (ni_710ae_ctx.element_ctx == NULL) {
        return FWK_E_NOMEM;
    }

    ni_710ae_ctx.element_count = element_count;

    return FWK_SUCCESS;
}

static int mod_ni_710ae_element_init(
    fwk_id_t element_id,
    unsigned int unused,
    const void *data)
{
    struct mod_ni_710ae_element_config *config;
    unsigned int idx;

    if (data == NULL) {
        return FWK_E_PARAM;
    }

    config = (struct mod_ni_710ae_element_config *)data;
    idx = fwk_id_get_element_idx(element_id);

    if (idx >= ni_710ae_ctx.element_count) {
        FWK_LOG_ERR(
            "[NI710AE] Invalid parameter as element id exceeds element count");
        return FWK_E_PARAM;
    }
    if (config->max_number_of_nodes > MOD_NI_710AE_MAX_NUMBER_OF_NODES) {
        FWK_LOG_ERR(
            "[NI710AE] Invalid parameter as max number of nodes exceeds "
            "limits");
        return FWK_E_PARAM;
    }
    ni_710ae_ctx.element_ctx[idx].element_name =
        fwk_module_get_element_name(element_id);
    ni_710ae_ctx.element_ctx[idx].config = config;

    /* Allocate memory for discovery tree according to max_number_of_nodes */
    ni_710ae_ctx.element_ctx[idx].discovery_mem_pool = fwk_mm_alloc_notrap(
        config->max_number_of_nodes,
        sizeof(ni_710ae_ctx.element_ctx[idx].discovery_tree));

    if (ni_710ae_ctx.element_ctx[idx].discovery_mem_pool == NULL) {
        FWK_LOG_ERR("[NI710AE] Memory allocation error during element init");
        return FWK_E_NOMEM;
    }

    /* Initialize root discovery tree*/
    memset(
        &ni_710ae_ctx.element_ctx[idx].discovery_tree,
        0,
        sizeof(ni_710ae_ctx.element_ctx[idx].discovery_tree));

    ni_710ae_ctx.element_ctx[idx].discovery_mem_pool_index = 0;

    return FWK_SUCCESS;
}

static int mod_ni_710ae_element_start(fwk_id_t element_id)
{
    struct mod_ni_710ae_element_config *config;
    struct ni710ae_discovery_tree_t *element_memory_pool;
    struct ni710ae_discovery_tree_t *discovery_tree;
    uint16_t *memory_index;
    int status;

    unsigned int ctx_element_idx = fwk_id_get_element_idx(element_id);
    struct mod_ni_710ae_element_ctx *ctx =
        &ni_710ae_ctx.element_ctx[ctx_element_idx];

    config = ctx->config;
    element_memory_pool = ctx->discovery_mem_pool;
    discovery_tree = &ctx->discovery_tree;
    memory_index = &ctx->discovery_mem_pool_index;

    discovery_tree->type = NI710AE_NODE_TYPE_GCN;
    discovery_tree->id = 0;
    discovery_tree->address = 0;
    discovery_tree->sibling = NULL;

    FWK_LOG_INFO("[NI710AE] Starting NCI discovery - '%s'", ctx->element_name);

    status = ni710ae_discovery(
        discovery_tree,
        config->periphbase_addr,
        element_memory_pool,
        memory_index,
        config->max_number_of_nodes);

    if (status != FWK_SUCCESS) {
        FWK_LOG_ERR("[NI710AE] Discovery is failed, error '%d'", status);
        return status;
    }
    FWK_LOG_INFO("[NI710AE] Discovery is successful - '%s'", ctx->element_name);

    FWK_LOG_INFO(
        "[NI710AE] Starting APU Programming - '%s'", ctx->element_name);

    status = program_ni_710ae_apu(
        config->apu_configs,
        config->apu_config_count,
        config->periphbase_addr,
        discovery_tree);
    if (status != FWK_SUCCESS) {
        FWK_LOG_ERR("[NI710AE] APU Programming is failed, error '%d'", status);
        return status;
    }
    FWK_LOG_INFO(
        "[NI710AE] APU Programming is successful - '%s'", ctx->element_name);

    return FWK_SUCCESS;
}

static int mod_ni_710ae_start(fwk_id_t id)
{
    if (fwk_id_get_type(id) == FWK_ID_TYPE_ELEMENT) {
        FWK_LOG_INFO(
            "Starting NI710AE element: %s", fwk_module_get_element_name(id));
        return mod_ni_710ae_element_start(id);
    }

    return FWK_SUCCESS;
}

const struct fwk_module module_ni_710ae = {
    .type = FWK_MODULE_TYPE_DRIVER,
    .init = mod_ni_710ae_init,
    .element_init = mod_ni_710ae_element_init,
    .start = mod_ni_710ae_start,
};
