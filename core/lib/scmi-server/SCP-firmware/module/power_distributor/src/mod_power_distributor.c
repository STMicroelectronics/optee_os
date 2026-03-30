/*
 * Arm SCP/MCP Software
 * Copyright (c) 2024-2025, Arm Limited and Contributors. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 * Description:
 *      Power Distributor
 */

#include "mod_power_distributor.h"

#include <interface_power_management.h>

#include <fwk_id.h>
#include <fwk_log.h>
#include <fwk_mm.h>
#include <fwk_module.h>
#include <fwk_module_idx.h>

#include <inttypes.h>

#define MOD_NAME "[PWR_DISTRIBUTOR] "

struct mod_power_distributor_data {
    uint32_t power_demand;
    uint32_t power_budget;
    uint32_t power_limit;
};

struct mod_power_distributor_node {
    struct mod_power_distributor_data data;
    size_t *children_idx_table;
    size_t children_count;
};

struct mod_power_distributor_domain_ctx {
    struct mod_power_distributor_node node;
    struct interface_power_management_api *controller_api;
    struct mod_power_distributor_domain_config *config;
};

struct mod_power_distributor_ctx {
    /* list of system domains context */
    struct mod_power_distributor_domain_ctx *domain;
    /* Number of domains */
    size_t domain_count;
    /* Table contains the tree traversing order of the domain (level by level)*/
    uint32_t *tree_traverse_order_table;
};

static struct mod_power_distributor_ctx power_distributor_ctx;

static int subtree_power_distribute(fwk_id_t subtree_root_id);
static int system_power_distribute(void);
static void calculate_power_attributes(void);
static int set_power_limit(fwk_id_t id, uint32_t power_limit);
static int set_power_demand(fwk_id_t id, uint32_t power_demand);

static struct mod_power_distributor_api distribution_api = {
    .subtree_power_distribute = subtree_power_distribute,
    .system_power_distribute = system_power_distribute,
};

struct interface_power_management_api power_management_api = {
    .set_power_demand = set_power_demand,
    .set_power_limit = set_power_limit,
};

typedef int (*calculate_deficit)(
    struct mod_power_distributor_domain_ctx *child_ctx);

static inline struct mod_power_distributor_domain_ctx *get_domain_ctx(
    size_t idx)
{
    return &power_distributor_ctx.domain[idx];
}

static inline bool domain_has_parent(
    struct mod_power_distributor_domain_ctx *domain_ctx)
{
    return domain_ctx->config->parent_idx < power_distributor_ctx.domain_count;
}

static inline void calculate_children_count(void)
{
    for (size_t i = 0; i < power_distributor_ctx.domain_count; i++) {
        struct mod_power_distributor_domain_ctx *domain_ctx = get_domain_ctx(i);
        if (domain_has_parent(domain_ctx)) {
            ++power_distributor_ctx.domain[domain_ctx->config->parent_idx]
                  .node.children_count;
        }
    }
}

static inline void allocate_children_table(void)
{
    for (size_t i = 0; i < power_distributor_ctx.domain_count; i++) {
        struct mod_power_distributor_domain_ctx *domain_ctx = get_domain_ctx(i);
        if (domain_ctx->node.children_count > 0) {
            domain_ctx->node.children_idx_table = fwk_mm_calloc(
                domain_ctx->node.children_count,
                sizeof(domain_ctx->node.children_idx_table[0]));
        }
    }
}

static inline void set_domain_relations(void)
{
    for (size_t i = 0; i < power_distributor_ctx.domain_count; i++) {
        struct mod_power_distributor_domain_ctx *domain_ctx = get_domain_ctx(i);
        domain_ctx->node.children_count = 0;
    }

    for (size_t i = 0; i < power_distributor_ctx.domain_count; i++) {
        struct mod_power_distributor_domain_ctx *domain_ctx = get_domain_ctx(i);
        if (domain_has_parent(domain_ctx)) {
            struct mod_power_distributor_domain_ctx *parent_ctx =
                get_domain_ctx(domain_ctx->config->parent_idx);
            parent_ctx->node
                .children_idx_table[parent_ctx->node.children_count++] = i;
        }
    }
}

static inline bool is_element_id_valid(fwk_id_t id)
{
    return fwk_id_get_module_idx(id) == FWK_MODULE_IDX_POWER_DISTRIBUTOR &&
        fwk_id_get_element_idx(id) < power_distributor_ctx.domain_count;
}

static inline void construct_tree(void)
{
    calculate_children_count();
    allocate_children_table();
    set_domain_relations();
}

static inline int construct_tree_traverse_order_table(void)
{
    uint32_t traverse_idx = 0;
    /* Find the root */
    for (size_t i = 0; i < power_distributor_ctx.domain_count; ++i) {
        if (domain_has_parent(&power_distributor_ctx.domain[i]) == false) {
            if (traverse_idx > 0) {
                /* Multi roots are not allowed as traverse_idx > 0 means
                 * Root has already been detected.
                 */
                FWK_LOG_ERR(MOD_NAME "Multiple roots are not allowed");
                return FWK_E_DATA;
            }
            power_distributor_ctx.tree_traverse_order_table[traverse_idx++] = i;
        }
    }

    if (traverse_idx == 0) {
        /* No root found */
        FWK_LOG_ERR(MOD_NAME "System must have a root");
        return FWK_E_DATA;
    }

    /* Store tree traverse order */
    for (size_t i = 0; i < power_distributor_ctx.domain_count; ++i) {
        struct mod_power_distributor_domain_ctx *parent_ctx =
            &power_distributor_ctx
                 .domain[power_distributor_ctx.tree_traverse_order_table[i]];
        for (size_t j = 0; j < parent_ctx->node.children_count; ++j) {
            /* When reaching the end of the traverse order table.
             * Domains are consider leaves and should not have children.
             */
            if (traverse_idx >= power_distributor_ctx.domain_count) {
                FWK_LOG_ERR(MOD_NAME "Incorrect Domain relations");
                return FWK_E_DATA;
            }
            power_distributor_ctx.tree_traverse_order_table[traverse_idx++] =
                parent_ctx->node.children_idx_table[j];
        }
    }

    return FWK_SUCCESS;
}

static int subtree_power_distribute(fwk_id_t subtree_root_id)
{
    return FWK_E_SUPPORT;
}

static uint32_t calculate_domain_limit(
    struct mod_power_distributor_domain_ctx *domain_ctx)
{
    uint32_t children_limit = 0;

    for (size_t i = 0; i < domain_ctx->node.children_count; i++) {
        size_t child_idx = domain_ctx->node.children_idx_table[i];

        struct mod_power_distributor_domain_ctx *child_ctx =
            get_domain_ctx(child_idx);

        uint32_t sum = children_limit + child_ctx->node.data.power_limit;
        children_limit = (sum < children_limit) ? UINT32_MAX : sum;

        /* No need to check further if accumulator reaches no limit, i.e. max
         * power */
        if (children_limit == UINT32_MAX) {
            break;
        }
    }

    return FWK_MIN(domain_ctx->node.data.power_limit, children_limit);
}

static uint32_t calculate_domain_demand(
    struct mod_power_distributor_domain_ctx *domain_ctx)
{
    uint32_t children_demand = 0;

    for (size_t i = 0; i < domain_ctx->node.children_count; i++) {
        size_t child_idx = domain_ctx->node.children_idx_table[i];

        struct mod_power_distributor_domain_ctx *child_ctx =
            get_domain_ctx(child_idx);

        uint32_t sum = children_demand + child_ctx->node.data.power_demand;
        children_demand = (sum < children_demand) ? UINT32_MAX : sum;

        /* No need to check further if accumulator reaches maximum power */
        if (children_demand == UINT32_MAX) {
            break;
        }
    }

    return FWK_MAX(domain_ctx->node.data.power_demand, children_demand);
}

static void calculate_power_attributes(void)
{
    size_t domain_count = power_distributor_ctx.domain_count;

    for (int i = domain_count - 1; i >= 0; i--) {
        uint32_t domain_idx =
            power_distributor_ctx.tree_traverse_order_table[i];
        struct mod_power_distributor_domain_ctx *domain_ctx =
            get_domain_ctx(domain_idx);

        if (domain_ctx->node.children_count > 0u) {
            domain_ctx->node.data.power_limit =
                calculate_domain_limit(domain_ctx);
            domain_ctx->node.data.power_demand =
                calculate_domain_demand(domain_ctx);
        }
    }
}

static int calculate_base_allocation_deficit(
    struct mod_power_distributor_domain_ctx *child_ctx)
{
    return FWK_MIN(
               child_ctx->node.data.power_demand,
               child_ctx->node.data.power_limit) -
        child_ctx->node.data.power_budget;
}

static int calculate_extra_allocation_deficit(
    struct mod_power_distributor_domain_ctx *child_ctx)
{
    return child_ctx->node.data.power_limit - child_ctx->node.data.power_budget;
}

static size_t count_eligible_children(
    struct mod_power_distributor_domain_ctx *domain_ctx,
    calculate_deficit calc_deficit)
{
    size_t eligible_count = 0u;
    uint32_t deficit;
    size_t i;

    for (i = 0u; i < domain_ctx->node.children_count; i++) {
        uint32_t child_idx = domain_ctx->node.children_idx_table[i];
        struct mod_power_distributor_domain_ctx *child_ctx =
            get_domain_ctx(child_idx);

        deficit = calc_deficit(child_ctx);

        if (deficit > 0u) {
            eligible_count++;
        }
    }
    return eligible_count;
}

static void allocate_power_waterfill(
    struct mod_power_distributor_domain_ctx *domain_ctx,
    uint32_t *remaining_budget,
    calculate_deficit calc_deficit,
    size_t eligible_count)
{
    uint32_t even_share;
    uint32_t deficit;
    size_t i;

    even_share = (*remaining_budget > eligible_count) ?
        (*remaining_budget / eligible_count) :
        1u;

    /* Allocate power to each eligible child */
    for (i = 0u; i < domain_ctx->node.children_count; i++) {
        if (*remaining_budget == 0u) {
            break;
        }

        uint32_t child_idx = domain_ctx->node.children_idx_table[i];
        struct mod_power_distributor_domain_ctx *child_ctx =
            get_domain_ctx(child_idx);

        deficit = calc_deficit(child_ctx);

        if (deficit == 0u) {
            continue;
        }

        uint32_t power_grant = FWK_MIN(deficit, even_share);
        child_ctx->node.data.power_budget += power_grant;
        *remaining_budget -= power_grant;
    }
}

static void distribute_power(
    struct mod_power_distributor_domain_ctx *domain_ctx,
    uint32_t *remaining_budget,
    calculate_deficit calc_deficit)
{
    size_t eligible_count;

    while (*remaining_budget != 0u) {
        /* Count eligible children for this phase */
        eligible_count = count_eligible_children(domain_ctx, calc_deficit);

        if (eligible_count == 0u) {
            break;
        }

        allocate_power_waterfill(
            domain_ctx, remaining_budget, calc_deficit, eligible_count);
    }
}

static int domain_power_distribute(
    struct mod_power_distributor_domain_ctx *domain_ctx)
{
    uint32_t remaining_budget = domain_ctx->node.data.power_budget;

    if (domain_ctx->node.children_count == 0) {
        return FWK_SUCCESS;
    }

    /* Phase 1: Base allocation */
    distribute_power(
        domain_ctx, &remaining_budget, &calculate_base_allocation_deficit);

    /* Phase 2: Extra allocation */
    distribute_power(
        domain_ctx, &remaining_budget, &calculate_extra_allocation_deficit);

    return (remaining_budget > 0u) ? FWK_E_DATA : FWK_SUCCESS;
}

static int set_budgets()
{
    for (size_t i = 0; i < power_distributor_ctx.domain_count; ++i) {
        struct mod_power_distributor_domain_ctx *domain_ctx =
            &power_distributor_ctx.domain[i];
        if (domain_ctx->controller_api != NULL) {
            FWK_LOG_DEBUG(
                MOD_NAME "Grant power for %s",
                fwk_module_get_element_name(
                    FWK_ID_ELEMENT(FWK_MODULE_IDX_POWER_DISTRIBUTOR, i)));
            domain_ctx->controller_api->set_power_limit(
                domain_ctx->config->controller_id,
                domain_ctx->node.data.power_budget);
        }
    }

    return FWK_SUCCESS;
}

static inline void clear_domains_budget(void)
{
    for (size_t i = 0; i < power_distributor_ctx.domain_count; ++i) {
        struct mod_power_distributor_domain_ctx *domain_ctx = get_domain_ctx(i);
        domain_ctx->node.data.power_budget = 0;
    }
}

static inline void set_root_budget(void)
{
    uint32_t root_idx = power_distributor_ctx.tree_traverse_order_table[0];
    struct mod_power_distributor_domain_ctx *root_ctx =
        get_domain_ctx(root_idx);

    root_ctx->node.data.power_budget = root_ctx->node.data.power_limit;
}

static inline int domains_power_distribute(void)
{
    int status = FWK_SUCCESS;

    for (size_t i = 0; i < power_distributor_ctx.domain_count; ++i) {
        uint32_t domain_idx =
            power_distributor_ctx.tree_traverse_order_table[i];
        struct mod_power_distributor_domain_ctx *domain_ctx =
            get_domain_ctx(domain_idx);

        fwk_assert(domain_ctx);

        status = domain_power_distribute(domain_ctx);
        if (status != FWK_SUCCESS) {
            fwk_id_t domain_id =
                FWK_ID_ELEMENT(FWK_MODULE_IDX_POWER_DISTRIBUTOR, domain_idx);
            FWK_LOG_ERR(
                MOD_NAME "Failed to distribute domain %s (index: %" PRIu32 ")",
                fwk_module_get_element_name(domain_id),
                domain_idx);
            break;
        }
    }
    return status;
}

static int system_power_distribute(void)
{
    int status = FWK_SUCCESS;

    calculate_power_attributes();
    clear_domains_budget();
    set_root_budget();

    status = domains_power_distribute();
    if (status != FWK_SUCCESS) {
        clear_domains_budget();
        return status;
    }

    return set_budgets();
}

int set_power_limit(fwk_id_t id, uint32_t power_limit)
{
    if (!is_element_id_valid(id)) {
        return FWK_E_PARAM;
    }

    struct mod_power_distributor_domain_ctx *domain_ctx =
        &power_distributor_ctx.domain[fwk_id_get_element_idx(id)];
    domain_ctx->node.data.power_limit = power_limit;
    return FWK_SUCCESS;
}

int set_power_demand(fwk_id_t id, uint32_t power_demand)
{
    if (!is_element_id_valid(id)) {
        return FWK_E_PARAM;
    }

    struct mod_power_distributor_domain_ctx *domain_ctx =
        &power_distributor_ctx.domain[fwk_id_get_element_idx(id)];
    domain_ctx->node.data.power_demand = power_demand;
    return FWK_SUCCESS;
}

/*
 * Framework handlers
 */

static int power_distributor_init(
    fwk_id_t module_id,
    unsigned int element_count,
    const void *config_data)
{
    if (element_count == 0U ||
        !fwk_id_is_equal(
            module_id, FWK_ID_MODULE(FWK_MODULE_IDX_POWER_DISTRIBUTOR))) {
        return FWK_E_PARAM;
    }

    power_distributor_ctx.domain_count = element_count;
    power_distributor_ctx.domain =
        fwk_mm_calloc(element_count, sizeof(power_distributor_ctx.domain[0]));

    power_distributor_ctx.tree_traverse_order_table = fwk_mm_alloc(
        element_count,
        sizeof(power_distributor_ctx.tree_traverse_order_table[0]));
    fwk_assert(power_distributor_ctx.tree_traverse_order_table != NULL);

    memset(
        power_distributor_ctx.tree_traverse_order_table,
        MOD_POWER_DISTRIBUTOR_DOMAIN_PARENT_IDX_NONE,
        element_count *
            sizeof(power_distributor_ctx.tree_traverse_order_table[0]));

    return FWK_SUCCESS;
}

static int power_distributor_element_init(
    fwk_id_t element_id,
    unsigned int sub_element_count,
    const void *data)
{
    unsigned int element_idx = fwk_id_get_element_idx(element_id);
    struct mod_power_distributor_domain_ctx *domain_ctx =
        get_domain_ctx(element_idx);

    domain_ctx->node.data.power_demand = 0;
    domain_ctx->node.data.power_limit = NO_POWER_LIMIT;
    domain_ctx->node.data.power_budget = 0;
    domain_ctx->controller_api = NULL;
    domain_ctx->config = (struct mod_power_distributor_domain_config *)data;
    domain_ctx->node.children_idx_table = NULL;
    domain_ctx->node.children_count = 0;

    return FWK_SUCCESS;
}

static int power_distributor_post_init(fwk_id_t module_id)
{
    construct_tree();
    return construct_tree_traverse_order_table();
}

static int power_distributor_bind(fwk_id_t id, unsigned int round)
{
    /* bind only elements and only in round 0 */
    if (round != 0 || !fwk_id_is_type(id, FWK_ID_TYPE_ELEMENT)) {
        return FWK_SUCCESS;
    }

    int status = FWK_SUCCESS;
    struct mod_power_distributor_domain_ctx *domain_ctx =
        get_domain_ctx(fwk_id_get_element_idx(id));

    if (fwk_optional_id_is_defined(domain_ctx->config->controller_api_id) &&
        !fwk_id_is_equal(domain_ctx->config->controller_api_id, FWK_ID_NONE)) {
        status = fwk_module_bind(
            fwk_id_build_module_id(domain_ctx->config->controller_api_id),
            domain_ctx->config->controller_api_id,
            &domain_ctx->controller_api);
    }

    return status;
}

static int power_distributor_process_bind_request(
    fwk_id_t source_id,
    fwk_id_t target_id,
    fwk_id_t api_id,
    const void **api)
{
    if ((enum fwk_module_idx)fwk_id_get_module_idx(api_id) !=
        FWK_MODULE_IDX_POWER_DISTRIBUTOR) {
        return FWK_E_PARAM;
    }

    enum mod_power_distributor_api_idx api_idx =
        (enum mod_power_distributor_api_idx)fwk_id_get_api_idx(api_id);

    switch (api_idx) {
    case MOD_POWER_DISTRIBUTOR_API_IDX_DISTRIBUTION:
        *api = &distribution_api;
        break;
    case MOD_POWER_DISTRIBUTOR_API_IDX_POWER_MANAGEMENT:
        *api = &power_management_api;
        break;
    default:
        return FWK_E_PARAM;
    }

    return FWK_SUCCESS;
}

static int power_distributor_start(fwk_id_t id)
{
    return FWK_SUCCESS;
}

/* power_distributor module definition */
const struct fwk_module module_power_distributor = {
    .type = FWK_MODULE_TYPE_SERVICE,
    .api_count = MOD_POWER_DISTRIBUTOR_API_IDX_COUNT,
    .init = power_distributor_init,
    .element_init = power_distributor_element_init,
    .post_init = power_distributor_post_init,
    .bind = power_distributor_bind,
    .start = power_distributor_start,
    .process_bind_request = power_distributor_process_bind_request,
};
