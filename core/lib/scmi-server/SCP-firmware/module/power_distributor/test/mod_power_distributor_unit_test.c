/*
 * Arm SCP/MCP Software
 * Copyright (c) 2024-2025, Arm Limited and Contributors. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 * Description:
 *     Power capping module unit test.
 */

#include "config_power_distributor.h"
#include "scp_unity.h"
#include "unity.h"

#include <Mockfwk_mm.h>
#include <Mockfwk_module.h>
#include <Mockmod_power_distributor_extra.h>

#include <mod_power_distributor.h>
#include <mod_power_distributor_extra.h>

#include <interface_power_management.h>

#include UNIT_TEST_SRC

struct mod_power_distributor_domain_ctx test_ctx_table[TEST_DOMAIN_COUNT];
uint32_t test_tree_traverse_order_table[TEST_DOMAIN_COUNT];

struct interface_power_management_api test_power_management_api = {
    .get_power_limit = &mock_get_power_limit,
    .set_power_limit = &mock_set_power_limit,
    .set_power_demand = &mock_set_power_demand,
};

void setUp(void)
{
    memset((void *)test_ctx_table, 0U, sizeof(test_ctx_table));
    memset(
        (void *)test_tree_traverse_order_table,
        TEST_DOMAIN_COUNT,
        sizeof(test_tree_traverse_order_table));

    power_distributor_ctx.domain = test_ctx_table;
    power_distributor_ctx.tree_traverse_order_table =
        test_tree_traverse_order_table;

    power_distributor_ctx.domain_count = TEST_DOMAIN_COUNT;
    for (unsigned int i = 0U; i < TEST_DOMAIN_COUNT; i++) {
        if (fwk_optional_id_is_defined(
                test_power_distributor_domain_config[i].controller_id)) {
            power_distributor_ctx.domain[i].controller_api =
                &test_power_management_api;
        }
        power_distributor_ctx.domain[i].config =
            &test_power_distributor_domain_config[i];
    }
    Mockfwk_mm_Init();
    Mockmod_power_distributor_extra_Init();
}

void utest_power_distributor_init_success(void)
{
    int status;
    unsigned int element_count = 4;
    uint32_t test_tree_traverse_order_table[4] = { 0, 1, 2, 3 };
    uint32_t test_tree_traverse_order_table_init_values[4] = { -1, -1, -1, -1 };
    struct mod_power_distributor_domain_ctx domain[4] = { 0 };
    power_distributor_ctx.domain_count = 0;

    fwk_mm_calloc_ExpectAndReturn(
        element_count, sizeof(power_distributor_ctx.domain[0]), &domain);

    fwk_mm_alloc_ExpectAndReturn(
        element_count,
        sizeof(power_distributor_ctx.tree_traverse_order_table[0]),
        test_tree_traverse_order_table);

    status = power_distributor_init(
        FWK_ID_MODULE(FWK_MODULE_IDX_POWER_DISTRIBUTOR), element_count, NULL);

    TEST_ASSERT_EQUAL(element_count, power_distributor_ctx.domain_count);
    TEST_ASSERT_EQUAL(domain, power_distributor_ctx.domain);

    TEST_ASSERT_EQUAL(
        test_tree_traverse_order_table,
        power_distributor_ctx.tree_traverse_order_table);
    TEST_ASSERT_EQUAL_HEX_ARRAY(
        test_tree_traverse_order_table_init_values,
        test_tree_traverse_order_table,
        element_count);

    TEST_ASSERT_EQUAL(FWK_SUCCESS, status);
}

void utest_power_distributor_element_init_success(void)
{
    int status;
    fwk_id_t element_id;
    static struct mod_power_distributor_domain_config
        domains_config[TEST_DOMAIN_COUNT] = { 0 };

    for (unsigned int index = 0U; index < TEST_DOMAIN_COUNT; index++) {
        element_id = FWK_ID_ELEMENT(FWK_MODULE_IDX_POWER_DISTRIBUTOR, index);

        status = power_distributor_element_init(
            element_id, 30U, &domains_config[index]);

        TEST_ASSERT_EQUAL_PTR(
            &domains_config[index], power_distributor_ctx.domain[index].config);
        TEST_ASSERT_EQUAL_PTR(
            NULL, power_distributor_ctx.domain[index].controller_api);
        TEST_ASSERT_EQUAL_PTR(
            NULL, power_distributor_ctx.domain[index].node.children_idx_table);
        TEST_ASSERT_EQUAL(
            0, power_distributor_ctx.domain[index].node.children_count);
        TEST_ASSERT_EQUAL(
            0, power_distributor_ctx.domain[index].node.data.power_demand);
        TEST_ASSERT_EQUAL(
            0, power_distributor_ctx.domain[index].node.data.power_budget);
        TEST_ASSERT_EQUAL(
            NO_POWER_LIMIT,
            power_distributor_ctx.domain[index].node.data.power_limit);
        TEST_ASSERT_EQUAL(FWK_SUCCESS, status);
    }
}

void utest_mod_distributor_bind_round_0(void)
{
    int status;
    unsigned int round = 0U;

    for (unsigned int index = 0U; index < TEST_DOMAIN_COUNT; index++) {
        fwk_id_t domain_id =
            FWK_ID_ELEMENT_INIT(FWK_MODULE_IDX_POWER_DISTRIBUTOR, index);
        struct mod_power_distributor_domain_ctx *domain_ctx =
            &(power_distributor_ctx.domain[index]);

        if (fwk_optional_id_is_defined(domain_ctx->config->controller_api_id) &&
            !fwk_id_is_equal(
                domain_ctx->config->controller_api_id, FWK_ID_NONE)) {
            fwk_module_bind_ExpectAndReturn(
                fwk_id_build_module_id(domain_ctx->config->controller_api_id),
                domain_ctx->config->controller_api_id,
                &domain_ctx->controller_api,
                FWK_SUCCESS);
        }

        status = power_distributor_bind(domain_id, round);

        TEST_ASSERT_EQUAL(FWK_SUCCESS, status);
    }
}

void utest_mod_distributor_bind_invalid_round(void)
{
    int status;
    unsigned int round = 1U;

    for (unsigned int index = 0U; index < TEST_DOMAIN_COUNT; index++) {
        fwk_id_t domain_id =
            FWK_ID_ELEMENT_INIT(FWK_MODULE_IDX_POWER_DISTRIBUTOR, index);

        status = power_distributor_bind(domain_id, round);

        TEST_ASSERT_EQUAL(FWK_SUCCESS, status);
    }
}

void utest_mod_distributor_process_bind_request(void)
{
    int status;
    fwk_id_t dummy_id = FWK_ID_NONE;
    const void *api = NULL;

    status = power_distributor_process_bind_request(
        dummy_id,
        dummy_id,
        FWK_ID_API(
            FWK_MODULE_IDX_POWER_DISTRIBUTOR,
            MOD_POWER_DISTRIBUTOR_API_IDX_DISTRIBUTION),
        &api);

    TEST_ASSERT_EQUAL_PTR(&distribution_api, api);
    TEST_ASSERT_EQUAL(FWK_SUCCESS, status);
}

void utest_mod_distributor_process_bind_request_invalid_api(void)
{
    int status;
    fwk_id_t dummy_id = FWK_ID_NONE;
    enum mod_power_distributor_api_idx invalid_api_id =
        MOD_POWER_DISTRIBUTOR_API_IDX_COUNT;
    const void *api = NULL;

    status = power_distributor_process_bind_request(
        dummy_id,
        dummy_id,
        FWK_ID_API(FWK_MODULE_IDX_POWER_DISTRIBUTOR, invalid_api_id),
        &api);

    TEST_ASSERT_EQUAL(FWK_E_PARAM, status);
}

void utest_mod_distributor_process_bind_request_invalid_module(void)
{
    int status;
    fwk_id_t dummy_id = FWK_ID_NONE;
    enum fwk_module_idx invalid_module_idx = FWK_MODULE_IDX_COUNT;
    const void *api = NULL;

    status = power_distributor_process_bind_request(
        dummy_id,
        dummy_id,
        FWK_ID_API(
            invalid_module_idx, MOD_POWER_DISTRIBUTOR_API_IDX_DISTRIBUTION),
        &api);

    TEST_ASSERT_EQUAL(FWK_E_PARAM, status);
}

void utest_mod_distributor_post_init_success(void)
{
    int status = FWK_E_DATA;
    struct mod_power_distributor_domain_config config[TEST_DOMAIN_COUNT] = {
        [TEST_DOMAIN_SOC] = { .parent_idx = TEST_DOMAIN_NONE,},
        [TEST_DOMAIN_CPU] = { .parent_idx = TEST_DOMAIN_SOC, },
        [TEST_DOMAIN_GPU] = { .parent_idx = TEST_DOMAIN_SOC, },
        [TEST_DOMAIN_CPU_BIG] = { .parent_idx = TEST_DOMAIN_CPU, },
        [TEST_DOMAIN_CPU_LITTLE] = { .parent_idx = TEST_DOMAIN_CPU, },
    };
    size_t children_of_soc[2];
    size_t children_of_cpu[2];
    power_distributor_ctx.domain_count = TEST_DOMAIN_COUNT;

    for (size_t i = 0; i < TEST_DOMAIN_COUNT; ++i) {
        power_distributor_ctx.domain[i].config = &config[i];
    }

    uint32_t test_tree_traverse_order_table[TEST_DOMAIN_COUNT];
    power_distributor_ctx.tree_traverse_order_table =
        test_tree_traverse_order_table;

    fwk_mm_calloc_ExpectAndReturn(
        2, sizeof(children_of_soc[0]), &children_of_soc);
    fwk_mm_calloc_ExpectAndReturn(
        2, sizeof(children_of_cpu[0]), &children_of_cpu);

    status = power_distributor_post_init(
        FWK_ID_MODULE(FWK_MODULE_IDX_POWER_DISTRIBUTOR));

    TEST_ASSERT_EQUAL(FWK_SUCCESS, status);
    TEST_ASSERT_EQUAL_PTR(
        &children_of_soc,
        power_distributor_ctx.domain[TEST_DOMAIN_SOC].node.children_idx_table);
    TEST_ASSERT_EQUAL(
        2, power_distributor_ctx.domain[TEST_DOMAIN_SOC].node.children_count);
    TEST_ASSERT_EQUAL_PTR(
        &children_of_cpu,
        power_distributor_ctx.domain[TEST_DOMAIN_CPU].node.children_idx_table);
    TEST_ASSERT_EQUAL(
        2, power_distributor_ctx.domain[TEST_DOMAIN_CPU].node.children_count);
    TEST_ASSERT_EQUAL_PTR(
        NULL,
        power_distributor_ctx.domain[TEST_DOMAIN_GPU].node.children_idx_table);
    TEST_ASSERT_EQUAL(
        0, power_distributor_ctx.domain[TEST_DOMAIN_GPU].node.children_count);
    TEST_ASSERT_EQUAL_PTR(
        NULL,
        power_distributor_ctx.domain[TEST_DOMAIN_CPU_BIG]
            .node.children_idx_table);
    TEST_ASSERT_EQUAL(
        0,
        power_distributor_ctx.domain[TEST_DOMAIN_CPU_BIG].node.children_count);
    TEST_ASSERT_EQUAL_PTR(
        NULL,
        power_distributor_ctx.domain[TEST_DOMAIN_CPU_LITTLE]
            .node.children_idx_table);
    TEST_ASSERT_EQUAL(
        0,
        power_distributor_ctx.domain[TEST_DOMAIN_CPU_LITTLE]
            .node.children_count);

    uint32_t expected_tree_traverse_order_table[] = {
        TEST_DOMAIN_SOC,     TEST_DOMAIN_CPU,        TEST_DOMAIN_GPU,
        TEST_DOMAIN_CPU_BIG, TEST_DOMAIN_CPU_LITTLE,
    };

    TEST_ASSERT_EQUAL_HEX32_ARRAY(
        expected_tree_traverse_order_table,
        power_distributor_ctx.tree_traverse_order_table,
        TEST_DOMAIN_COUNT);
}

void utest_mod_distributor_set_power_limit_demand_api_success(void)
{
    int status = FWK_E_DATA;
    for (size_t i = 0; i < TEST_DOMAIN_COUNT; ++i) {
        fwk_id_t elem_id = FWK_ID_ELEMENT(FWK_MODULE_IDX_POWER_DISTRIBUTOR, i);
        status = set_power_limit(elem_id, i);
        TEST_ASSERT_EQUAL(
            i, power_distributor_ctx.domain[i].node.data.power_limit);
        TEST_ASSERT_EQUAL(FWK_SUCCESS, status);
        status = set_power_demand(elem_id, i);
        TEST_ASSERT_EQUAL(
            i, power_distributor_ctx.domain[i].node.data.power_demand);
        TEST_ASSERT_EQUAL(FWK_SUCCESS, status);
    }
}

void utest_mod_distributor_set_power_limit_demand_api_invalid_params(void)
{
    int status = FWK_E_DATA;
    fwk_id_t invalid_elem_id =
        FWK_ID_ELEMENT(FWK_MODULE_IDX_POWER_DISTRIBUTOR, TEST_DOMAIN_COUNT);
    fwk_id_t invalid_elem_id2 = FWK_ID_ELEMENT(
        FWK_MODULE_IDX_POWER_DISTRIBUTOR, TEST_DOMAIN_COUNT + 10);
    fwk_id_t invalid_module_elem_id =
        FWK_ID_ELEMENT(FWK_MODULE_IDX_COUNT, TEST_DOMAIN_CPU);

    status = set_power_limit(invalid_elem_id, 0);
    TEST_ASSERT_EQUAL(FWK_E_PARAM, status);
    status = set_power_demand(invalid_elem_id, 0);
    TEST_ASSERT_EQUAL(FWK_E_PARAM, status);

    status = set_power_limit(invalid_elem_id2, 0);
    TEST_ASSERT_EQUAL(FWK_E_PARAM, status);
    status = set_power_demand(invalid_elem_id2, 0);
    TEST_ASSERT_EQUAL(FWK_E_PARAM, status);

    status = set_power_limit(invalid_module_elem_id, 0);
    TEST_ASSERT_EQUAL(FWK_E_PARAM, status);
    status = set_power_demand(invalid_module_elem_id, 0);
    TEST_ASSERT_EQUAL(FWK_E_PARAM, status);
}

void utest_mod_distributor_set_budgets(void)
{
    int status = FWK_E_INIT;
    for (size_t i = 0; i < power_distributor_ctx.domain_count; ++i) {
        struct mod_power_distributor_domain_ctx *domain_ctx =
            &power_distributor_ctx.domain[i];
        domain_ctx->node.data.power_budget = 0xDEADBEEF + i;
    }

    for (size_t i = 0; i < power_distributor_ctx.domain_count; ++i) {
        struct mod_power_distributor_domain_ctx *domain_ctx =
            &power_distributor_ctx.domain[i];

        if (domain_ctx->controller_api != NULL) {
            fwk_id_t elem_id =
                FWK_ID_ELEMENT_INIT(FWK_MODULE_IDX_CONTROLLER, i);
            fwk_module_get_element_name_ExpectAnyArgsAndReturn("");
            mock_set_power_limit_ExpectAndReturn(
                elem_id, domain_ctx->node.data.power_budget, FWK_SUCCESS);
        }
    }

    status = set_budgets();
    TEST_ASSERT_EQUAL(FWK_SUCCESS, status);
}

void utest_construct_tree_traverse_order_table_success(void)
{
    int status = FWK_E_INIT;
    size_t children_of_soc[] = { TEST_DOMAIN_CPU, TEST_DOMAIN_GPU };
    size_t children_of_cpu[] = { TEST_DOMAIN_CPU_BIG, TEST_DOMAIN_CPU_LITTLE };
    uint32_t expected_tree_traverse_order_table[] = {
        TEST_DOMAIN_SOC,     TEST_DOMAIN_CPU,        TEST_DOMAIN_GPU,
        TEST_DOMAIN_CPU_BIG, TEST_DOMAIN_CPU_LITTLE,
    };
    struct mod_power_distributor_domain_config config[TEST_DOMAIN_COUNT] = {
        [TEST_DOMAIN_SOC] = { .parent_idx = TEST_DOMAIN_NONE,},
        [TEST_DOMAIN_CPU] = { .parent_idx = TEST_DOMAIN_SOC, },
        [TEST_DOMAIN_GPU] = { .parent_idx = TEST_DOMAIN_SOC, },
        [TEST_DOMAIN_CPU_BIG] = { .parent_idx = TEST_DOMAIN_CPU, },
        [TEST_DOMAIN_CPU_LITTLE] = { .parent_idx = TEST_DOMAIN_CPU, },
    };
    struct mod_power_distributor_domain_ctx test_domains[TEST_DOMAIN_COUNT] = {
        0
    };
    uint32_t test_tree_traverse_order_table[TEST_DOMAIN_COUNT];

    power_distributor_ctx.domain = test_domains;
    power_distributor_ctx.domain_count = TEST_DOMAIN_COUNT;
    power_distributor_ctx.tree_traverse_order_table =
        test_tree_traverse_order_table;

    power_distributor_ctx.domain[TEST_DOMAIN_SOC].node.children_count =
        FWK_ARRAY_SIZE(children_of_soc);
    power_distributor_ctx.domain[TEST_DOMAIN_SOC].node.children_idx_table =
        children_of_soc;
    power_distributor_ctx.domain[TEST_DOMAIN_CPU].node.children_count =
        FWK_ARRAY_SIZE(children_of_cpu);
    power_distributor_ctx.domain[TEST_DOMAIN_CPU].node.children_idx_table =
        children_of_cpu;

    for (size_t i = 0; i < TEST_DOMAIN_COUNT; ++i) {
        power_distributor_ctx.domain[i].config = &config[i];
    }

    status = construct_tree_traverse_order_table();
    TEST_ASSERT_EQUAL(FWK_SUCCESS, status);

    TEST_ASSERT_EQUAL_HEX32_ARRAY(
        expected_tree_traverse_order_table,
        power_distributor_ctx.tree_traverse_order_table,
        TEST_DOMAIN_COUNT);
}

void utest_construct_tree_traverse_order_table_multipe_roots(void)
{
    int status = FWK_E_INIT;

    struct mod_power_distributor_domain_config config[TEST_DOMAIN_COUNT] = {
        [TEST_DOMAIN_SOC] = { .parent_idx = TEST_DOMAIN_NONE,},
        [TEST_DOMAIN_CPU] = { .parent_idx = TEST_DOMAIN_NONE, },
        [TEST_DOMAIN_GPU] = { .parent_idx = TEST_DOMAIN_SOC, },
        [TEST_DOMAIN_CPU_BIG] = { .parent_idx = TEST_DOMAIN_CPU, },
        [TEST_DOMAIN_CPU_LITTLE] = { .parent_idx = TEST_DOMAIN_CPU, },
    };

    struct mod_power_distributor_domain_ctx test_domains[TEST_DOMAIN_COUNT] = {
        0
    };
    uint32_t test_tree_traverse_order_table[TEST_DOMAIN_COUNT];

    power_distributor_ctx.domain = test_domains;
    power_distributor_ctx.domain_count = TEST_DOMAIN_COUNT;
    power_distributor_ctx.tree_traverse_order_table =
        test_tree_traverse_order_table;
    for (size_t i = 0; i < TEST_DOMAIN_COUNT; ++i) {
        power_distributor_ctx.domain[i].config = &config[i];
    }

    status = construct_tree_traverse_order_table();
    TEST_ASSERT_EQUAL(FWK_E_DATA, status);
}

void utest_construct_tree_traverse_order_table_exceeded(void)
{
    int status = FWK_E_INIT;
    size_t children_of_soc[] = { TEST_DOMAIN_CPU, TEST_DOMAIN_GPU };
    size_t children_of_cpu[] = { TEST_DOMAIN_CPU_BIG, TEST_DOMAIN_CPU_LITTLE };
    /* bad config */
    size_t children_of_gpu[] = { TEST_DOMAIN_CPU_BIG, TEST_DOMAIN_CPU_LITTLE };
    uint32_t expected_tree_traverse_order_table[] = {
        TEST_DOMAIN_SOC,     TEST_DOMAIN_CPU,        TEST_DOMAIN_GPU,
        TEST_DOMAIN_CPU_BIG, TEST_DOMAIN_CPU_LITTLE,
    };
    struct mod_power_distributor_domain_config config[TEST_DOMAIN_COUNT] = {
        [TEST_DOMAIN_SOC] = { .parent_idx = TEST_DOMAIN_NONE,},
        [TEST_DOMAIN_CPU] = { .parent_idx = TEST_DOMAIN_SOC, },
        [TEST_DOMAIN_GPU] = { .parent_idx = TEST_DOMAIN_SOC, },
        [TEST_DOMAIN_CPU_BIG] = { .parent_idx = TEST_DOMAIN_CPU, },
        [TEST_DOMAIN_CPU_LITTLE] = { .parent_idx = TEST_DOMAIN_CPU, },
    };
    struct mod_power_distributor_domain_ctx test_domains[TEST_DOMAIN_COUNT] = {
        0
    };
    uint32_t test_tree_traverse_order_table[TEST_DOMAIN_COUNT];

    power_distributor_ctx.domain = test_domains;
    power_distributor_ctx.domain_count = TEST_DOMAIN_COUNT;
    power_distributor_ctx.tree_traverse_order_table =
        test_tree_traverse_order_table;

    power_distributor_ctx.domain[TEST_DOMAIN_SOC].node.children_count =
        FWK_ARRAY_SIZE(children_of_soc);
    power_distributor_ctx.domain[TEST_DOMAIN_SOC].node.children_idx_table =
        children_of_soc;
    power_distributor_ctx.domain[TEST_DOMAIN_CPU].node.children_count =
        FWK_ARRAY_SIZE(children_of_cpu);
    power_distributor_ctx.domain[TEST_DOMAIN_CPU].node.children_idx_table =
        children_of_cpu;
    power_distributor_ctx.domain[TEST_DOMAIN_GPU].node.children_count =
        FWK_ARRAY_SIZE(children_of_gpu);
    power_distributor_ctx.domain[TEST_DOMAIN_GPU].node.children_idx_table =
        children_of_gpu;

    for (size_t i = 0; i < TEST_DOMAIN_COUNT; ++i) {
        power_distributor_ctx.domain[i].config = &config[i];
    }

    status = construct_tree_traverse_order_table();
    TEST_ASSERT_EQUAL(FWK_E_DATA, status);

    TEST_ASSERT_EQUAL_HEX32_ARRAY(
        expected_tree_traverse_order_table,
        power_distributor_ctx.tree_traverse_order_table,
        TEST_DOMAIN_COUNT);
}

void utest_construct_tree_traverse_order_table_no_root(void)
{
    int status = FWK_E_INIT;

    /* Invalid: No true root and forms a loop */
    struct mod_power_distributor_domain_config config[TEST_DOMAIN_COUNT] = {
        [TEST_DOMAIN_SOC] = { .parent_idx = TEST_DOMAIN_CPU, },
        [TEST_DOMAIN_CPU] = { .parent_idx = TEST_DOMAIN_GPU, },
        [TEST_DOMAIN_GPU] = { .parent_idx = TEST_DOMAIN_CPU_BIG, },
        [TEST_DOMAIN_CPU_BIG] = { .parent_idx = TEST_DOMAIN_CPU_LITTLE, },
        [TEST_DOMAIN_CPU_LITTLE] = { .parent_idx = TEST_DOMAIN_SOC, },
    };

    struct mod_power_distributor_domain_ctx test_domains[TEST_DOMAIN_COUNT] = {
        0
    };
    uint32_t test_tree_traverse_order_table[TEST_DOMAIN_COUNT];

    power_distributor_ctx.domain = test_domains;
    power_distributor_ctx.domain_count = TEST_DOMAIN_COUNT;
    power_distributor_ctx.tree_traverse_order_table =
        test_tree_traverse_order_table;

    for (size_t i = 0; i < TEST_DOMAIN_COUNT; ++i) {
        power_distributor_ctx.domain[i].config = &config[i];
    }

    status = construct_tree_traverse_order_table();
    TEST_ASSERT_EQUAL(FWK_E_DATA, status);
}

void utest_mod_distributor_calculate_power_attributes(void)
{
    size_t children_of_soc[] = { TEST_DOMAIN_CPU, TEST_DOMAIN_GPU };
    size_t children_of_cpu[] = { TEST_DOMAIN_CPU_BIG, TEST_DOMAIN_CPU_LITTLE };

    struct mod_power_distributor_domain_ctx test_domains[TEST_DOMAIN_COUNT] = {
        [TEST_DOMAIN_SOC] = { .node = { .children_idx_table = children_of_soc,
                                        .children_count = FWK_ARRAY_SIZE(children_of_soc),
                                        .data = { .power_limit = 25,
                                                  .power_demand = 20, }, }, },
        [TEST_DOMAIN_CPU] = { .node = { .children_idx_table = children_of_cpu,
                                        .children_count = FWK_ARRAY_SIZE(children_of_cpu),
                                        .data = { .power_limit = 9,
                                                  .power_demand = 17, }, }, },

        [TEST_DOMAIN_GPU] = { .node = { .children_idx_table = NULL,
                                        .children_count = 0,
                                        .data = { .power_limit = 10,
                                                  .power_demand = 20, }, }, },

        [TEST_DOMAIN_CPU_BIG] = { .node = { .children_idx_table = NULL,
                                            .children_count = 0,
                                            .data = { .power_limit = 6,
                                                      .power_demand = 5, }, }, },

        [TEST_DOMAIN_CPU_LITTLE] = { .node = { .children_idx_table = NULL,
                                               .children_count = 0,
                                               .data = { .power_limit = 5,
                                                         .power_demand = 10, }, }, },
    };

    uint32_t tree_traverse_order_table[TEST_DOMAIN_COUNT] = {
        TEST_DOMAIN_SOC,
        TEST_DOMAIN_CPU,
        TEST_DOMAIN_GPU,
        TEST_DOMAIN_CPU_BIG,
        TEST_DOMAIN_CPU_LITTLE
    };

    power_distributor_ctx.domain_count = TEST_DOMAIN_COUNT;
    power_distributor_ctx.domain = test_domains;
    power_distributor_ctx.tree_traverse_order_table = tree_traverse_order_table;

    calculate_power_attributes();

    TEST_ASSERT_EQUAL(
        19,
        power_distributor_ctx.domain[TEST_DOMAIN_SOC].node.data.power_limit);
    TEST_ASSERT_EQUAL(
        9, power_distributor_ctx.domain[TEST_DOMAIN_CPU].node.data.power_limit);
    TEST_ASSERT_EQUAL(
        6,
        power_distributor_ctx.domain[TEST_DOMAIN_CPU_BIG]
            .node.data.power_limit);
    TEST_ASSERT_EQUAL(
        5,
        power_distributor_ctx.domain[TEST_DOMAIN_CPU_LITTLE]
            .node.data.power_limit);
    TEST_ASSERT_EQUAL(
        10,
        power_distributor_ctx.domain[TEST_DOMAIN_GPU].node.data.power_limit);

    TEST_ASSERT_EQUAL(
        37,
        power_distributor_ctx.domain[TEST_DOMAIN_SOC].node.data.power_demand);
    TEST_ASSERT_EQUAL(
        17,
        power_distributor_ctx.domain[TEST_DOMAIN_CPU].node.data.power_demand);
    TEST_ASSERT_EQUAL(
        5,
        power_distributor_ctx.domain[TEST_DOMAIN_CPU_BIG]
            .node.data.power_demand);
    TEST_ASSERT_EQUAL(
        10,
        power_distributor_ctx.domain[TEST_DOMAIN_CPU_LITTLE]
            .node.data.power_demand);
    TEST_ASSERT_EQUAL(
        20,
        power_distributor_ctx.domain[TEST_DOMAIN_GPU].node.data.power_demand);
}

void utest_mod_distributor_calculate_power_attributes_max_power(void)
{
    size_t children_of_soc[] = { TEST_DOMAIN_CPU, TEST_DOMAIN_GPU };
    size_t children_of_cpu[] = { TEST_DOMAIN_CPU_BIG, TEST_DOMAIN_CPU_LITTLE };

    struct mod_power_distributor_domain_ctx test_domains[TEST_DOMAIN_COUNT] = {
        [TEST_DOMAIN_SOC] = { .node = { .children_idx_table = children_of_soc,
                                        .children_count = FWK_ARRAY_SIZE(children_of_soc),
                                        .data = { .power_limit = 25,
                                                  .power_demand = 14, }, }, },
        [TEST_DOMAIN_CPU] = { .node = { .children_idx_table = children_of_cpu,
                                        .children_count = FWK_ARRAY_SIZE(children_of_cpu),
                                        .data = { .power_limit = 9,
                                                  .power_demand = 17, }, }, },

        [TEST_DOMAIN_GPU] = { .node = { .children_idx_table = NULL,
                                        .children_count = 0,
                                        .data = { .power_limit = 10,
                                                  .power_demand = MAX_POWER, }, }, },

        [TEST_DOMAIN_CPU_BIG] = { .node = { .children_idx_table = NULL,
                                            .children_count = 0,
                                            .data = { .power_limit = 6,
                                                      .power_demand = 5, }, }, },

        [TEST_DOMAIN_CPU_LITTLE] = { .node = { .children_idx_table = NULL,
                                               .children_count = 0,
                                               .data = { .power_limit = NO_POWER_LIMIT,
                                                         .power_demand = 10, }, }, },
    };

    uint32_t tree_traverse_order_table[TEST_DOMAIN_COUNT] = {
        TEST_DOMAIN_SOC,
        TEST_DOMAIN_CPU,
        TEST_DOMAIN_GPU,
        TEST_DOMAIN_CPU_BIG,
        TEST_DOMAIN_CPU_LITTLE
    };

    power_distributor_ctx.domain_count = TEST_DOMAIN_COUNT;
    power_distributor_ctx.domain = test_domains;
    power_distributor_ctx.tree_traverse_order_table = tree_traverse_order_table;

    calculate_power_attributes();

    TEST_ASSERT_EQUAL(
        19,
        power_distributor_ctx.domain[TEST_DOMAIN_SOC].node.data.power_limit);
    TEST_ASSERT_EQUAL(
        9, power_distributor_ctx.domain[TEST_DOMAIN_CPU].node.data.power_limit);
    TEST_ASSERT_EQUAL(
        6,
        power_distributor_ctx.domain[TEST_DOMAIN_CPU_BIG]
            .node.data.power_limit);
    TEST_ASSERT_EQUAL(
        NO_POWER_LIMIT,
        power_distributor_ctx.domain[TEST_DOMAIN_CPU_LITTLE]
            .node.data.power_limit);
    TEST_ASSERT_EQUAL(
        10,
        power_distributor_ctx.domain[TEST_DOMAIN_GPU].node.data.power_limit);

    TEST_ASSERT_EQUAL(
        MAX_POWER,
        power_distributor_ctx.domain[TEST_DOMAIN_SOC].node.data.power_demand);
    TEST_ASSERT_EQUAL(
        17,
        power_distributor_ctx.domain[TEST_DOMAIN_CPU].node.data.power_demand);
    TEST_ASSERT_EQUAL(
        5,
        power_distributor_ctx.domain[TEST_DOMAIN_CPU_BIG]
            .node.data.power_demand);
    TEST_ASSERT_EQUAL(
        10,
        power_distributor_ctx.domain[TEST_DOMAIN_CPU_LITTLE]
            .node.data.power_demand);
    TEST_ASSERT_EQUAL(
        MAX_POWER,
        power_distributor_ctx.domain[TEST_DOMAIN_GPU].node.data.power_demand);
}

/* Macro to initialize a leaf domain node */
#define INIT_LEAF_NODE(demand, limit) \
    { \
        .node = { \
            .children_idx_table = NULL, \
            .children_count = 0, \
            .data = { .power_limit = (limit), \
                      .power_demand = (demand), \
                      .power_budget = 0 } \
        } \
    }
void utest_mod_distributor_domain_power_distribute(void)
{
    size_t test_vec_size = 8;
    uint32_t parent_budget[] = {
        90, 150, 160, 150, 150, 150, 140, 140,
    };
    uint32_t children_demand[8][4] = {
        { 30, 10, 10, 40 }, { 60, 40, 30, 20 }, { 50, 30, 20, 50 },
        { 60, 40, 30, 20 }, { 40, 40, 40, 40 }, { 50, 20, 60, 30 },
        { 50, 30, 20, 50 }, { 50, 30, 30, 40 },
    };
    uint32_t children_limit[8][4] = {
        { 20, 40, 40, 30 }, { 50, 25, 35, 45 }, { 45, 35, 25, 55 },
        { 50, 30, 25, 45 }, { 50, 50, 50, 50 }, { 10, 60, 60, 50 },
        { 45, 35, 25, 55 }, { 40, 30, 20, 50 },
    };
    uint32_t expected_children_budget[8][4] = {
        { 20, 20, 20, 30 }, { 50, 25, 35, 40 }, { 45, 35, 25, 55 },
        { 50, 30, 25, 45 }, { 38, 38, 37, 37 }, { 10, 35, 60, 45 },
        { 45, 30, 20, 45 }, { 40, 30, 20, 50 },
    };

    size_t soc_children[] = { TEST_DOMAIN_CPU,
                              TEST_DOMAIN_CPU_BIG,
                              TEST_DOMAIN_CPU_LITTLE,
                              TEST_DOMAIN_GPU };

    for (size_t t = 0; t < test_vec_size; t++) {
        struct mod_power_distributor_domain_ctx
            test_domains[TEST_DOMAIN_COUNT] = {
                [TEST_DOMAIN_SOC] = { .node = { .children_idx_table =
                                                    soc_children,
                                                .children_count =
                                                    FWK_ARRAY_SIZE(
                                                        soc_children),
                                                .data = { .power_budget =
                                                              parent_budget
                                                                  [t] } } },
                [TEST_DOMAIN_CPU] =
                    INIT_LEAF_NODE(children_demand[t][0], children_limit[t][0]),
                [TEST_DOMAIN_CPU_BIG] =
                    INIT_LEAF_NODE(children_demand[t][1], children_limit[t][1]),
                [TEST_DOMAIN_CPU_LITTLE] =
                    INIT_LEAF_NODE(children_demand[t][2], children_limit[t][2]),
                [TEST_DOMAIN_GPU] =
                    INIT_LEAF_NODE(children_demand[t][3], children_limit[t][3]),
            };

        power_distributor_ctx.domain = test_domains;

        (void)domain_power_distribute(
            &power_distributor_ctx.domain[TEST_DOMAIN_SOC]);

        for (size_t i = 0; i < FWK_ARRAY_SIZE(soc_children); i++) {
            size_t idx = soc_children[i];
            char message[50];
            sprintf(message, "TestVec[%ld] in domain %ld", t, i);
            TEST_ASSERT_EQUAL_MESSAGE(
                expected_children_budget[t][i],
                power_distributor_ctx.domain[idx].node.data.power_budget,
                message);
        }
    }
}
#undef INIT_LEAF_NODE
void utest_clear_domains_budget(void)
{
    struct mod_power_distributor_domain_ctx test_domains[TEST_DOMAIN_COUNT] = {
        0
    };

    power_distributor_ctx.domain_count = TEST_DOMAIN_COUNT;
    power_distributor_ctx.domain = test_domains;

    for (size_t i = 0; i < TEST_DOMAIN_COUNT; i++) {
        test_domains[i].node.data.power_budget = 100; /* Assign some value */
    }

    clear_domains_budget();

    for (size_t i = 0; i < TEST_DOMAIN_COUNT; i++) {
        TEST_ASSERT_EQUAL(0, test_domains[i].node.data.power_budget);
    }
}

void utest_set_root_budget(void)
{
    struct mod_power_distributor_domain_ctx test_domains[TEST_DOMAIN_COUNT] = {
        0
    };
    uint32_t tree_traverse_order_table[TEST_DOMAIN_COUNT];
    tree_traverse_order_table[0] = TEST_DOMAIN_SOC;

    power_distributor_ctx.domain_count = TEST_DOMAIN_COUNT;
    power_distributor_ctx.domain = test_domains;
    power_distributor_ctx.tree_traverse_order_table = tree_traverse_order_table;

    test_domains[TEST_DOMAIN_SOC].node.data.power_limit = 200;

    set_root_budget();
    TEST_ASSERT_EQUAL(
        200, test_domains[TEST_DOMAIN_SOC].node.data.power_budget);
}

void tearDown(void)
{
    Mockmod_power_distributor_extra_Verify();
    Mockfwk_mm_Verify();
    Mockmod_power_distributor_extra_Destroy();
}

int power_distributor_test_main(void)
{
    UNITY_BEGIN();

    RUN_TEST(utest_power_distributor_init_success);
    RUN_TEST(utest_power_distributor_element_init_success);
    RUN_TEST(utest_mod_distributor_bind_round_0);
    RUN_TEST(utest_mod_distributor_bind_invalid_round);
    RUN_TEST(utest_mod_distributor_process_bind_request);
    RUN_TEST(utest_mod_distributor_process_bind_request_invalid_api);
    RUN_TEST(utest_mod_distributor_process_bind_request_invalid_module);
    RUN_TEST(utest_mod_distributor_post_init_success);
    RUN_TEST(utest_mod_distributor_set_power_limit_demand_api_success);
    RUN_TEST(utest_mod_distributor_set_power_limit_demand_api_invalid_params);
    RUN_TEST(utest_mod_distributor_set_budgets);
    RUN_TEST(utest_construct_tree_traverse_order_table_success);
    RUN_TEST(utest_construct_tree_traverse_order_table_multipe_roots);
    RUN_TEST(utest_construct_tree_traverse_order_table_exceeded);
    RUN_TEST(utest_construct_tree_traverse_order_table_no_root);
    RUN_TEST(utest_mod_distributor_calculate_power_attributes);
    RUN_TEST(utest_mod_distributor_calculate_power_attributes_max_power);
    RUN_TEST(utest_mod_distributor_domain_power_distribute);
    RUN_TEST(utest_clear_domains_budget);
    RUN_TEST(utest_set_root_budget);

    return UNITY_END();
}

#if !defined(TEST_ON_TARGET)
int main(void)
{
    return power_distributor_test_main();
}
#endif
