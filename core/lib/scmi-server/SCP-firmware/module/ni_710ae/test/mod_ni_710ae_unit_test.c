/*
 * Arm SCP/MCP Software
 * Copyright (c) 2025, Arm Limited and Contributors. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include "Mockni_710ae_lib.h"
#include "internal/Mockfwk_id_internal.h"
#include "scp_unity.h"
#include "unity.h"

#include <Mockfwk_id.h>
#include <Mockfwk_mm.h>
#include <Mockfwk_module.h>

#include <mod_ni_710ae.h>

#ifdef TESTING_MOD_NI_710AE
#    include "Mockni_710ae_discovery_drv.h"
#endif // TESTING_MOD_NI_710AE

#ifdef TESTING_NI_710AE_DISCOVERY_DRV
#    include "ni_710ae_discovery_drv.h"
#endif // TESTING_NI_710AE_DISCOVERY_DRV

#include UNIT_TEST_SRC
#include "config_ni_710ae.h"

#if UINTPTR_MAX == 0xFFFFFFFFu
#    define TEST_ASSERT_EQUAL_UINTPTR(expected, actual) \
        TEST_ASSERT_EQUAL_UINT32((uint32_t)(expected), (uint32_t)(actual))
#else
#    define TEST_ASSERT_EQUAL_UINTPTR(expected, actual) \
        TEST_ASSERT_EQUAL_UINT64((uint64_t)(expected), (uint64_t)(actual))
#endif

void setUp(void)
{
    /* Do Nothing */
}

void tearDown(void)
{
    /* Do Nothing */
}

#ifdef TESTING_MOD_NI_710AE

/*!
 * \brief ni710ae unit test: mod_ni710ae_init(), zero element.
 *
 *  \details Handle case in mod_ni710ae_init() where element count is 0
 */
void utest_mod_ni_710ae_init_fail_no_element(void)
{
    int status;

    /* Test init by passing NULL pointer instead of data */
    status = mod_ni_710ae_init(FWK_ID_MODULE(FWK_MODULE_IDX_NI_710AE), 0, NULL);
    TEST_ASSERT_EQUAL(FWK_E_PARAM, status);
}

/*!
 * \brief ni710ae unit test: mod_ni710ae_init(), memory allocation failure
 *
 *  \details Handle case in mod_ni710ae_init() where memory allocation fails
 */
void utest_mod_ni_710ae_init_fail_no_memory(void)
{
    int status;

    fwk_mm_calloc_ExpectAndReturn(
        (unsigned int)DUMMY_NI_710AE_NCI_COUNT,
        sizeof(struct mod_ni_710ae_element_ctx),
        NULL);

    status = mod_ni_710ae_init(
        FWK_ID_MODULE(FWK_MODULE_IDX_NI_710AE),
        (unsigned int)DUMMY_NI_710AE_NCI_COUNT,
        NULL);
    TEST_ASSERT_EQUAL(FWK_E_NOMEM, status);
}

/*!
 * \brief ni710ae unit test: mod_ni710ae_init(), valid config data.
 *
 *  \details Handle case in mod_ni710ae_init() where valid config data is
 * passed.
 */
void utest_mod_ni_710ae_init_success(void)
{
    int status;
    struct mod_ni_710ae_element_ctx temp_element_ctx;

    fwk_mm_calloc_ExpectAndReturn(
        1, sizeof(struct mod_ni_710ae_element_ctx), &temp_element_ctx);

    status = mod_ni_710ae_init(FWK_ID_MODULE(FWK_MODULE_IDX_NI_710AE), 1, NULL);

    TEST_ASSERT_EQUAL(FWK_SUCCESS, status);
}

/*!
 * \brief ni710ae unit test: mod_ni710ae_element_init(), Invalid element index.
 *
 *  \details Handle case in mod_ni710ae_element_init() where invalid element
 * index is passed.
 */
void utest_mod_ni_710ae_element_init_fail_invalid_index(void)
{
    int status;

    static struct mod_ni_710ae_element_config temp_element_config = {
        .max_number_of_nodes = 1,
        .periphbase_addr = 0xDEDEDEDE,
    };

    static struct mod_ni_710ae_element_ctx temp_element_ctx;
    ni_710ae_ctx.element_ctx = &temp_element_ctx;

    fwk_id_t invalid_element_id = FWK_ID_ELEMENT(FWK_MODULE_IDX_NI_710AE, 2);
    ni_710ae_ctx.element_count = 1;

    fwk_module_get_element_name_IgnoreAndReturn("Test Element");
    fwk_id_get_element_idx_ExpectAndReturn(invalid_element_id, 2);

    fwk_mm_alloc_notrap_ExpectAndReturn(
        temp_element_config.max_number_of_nodes,
        sizeof(struct ni710ae_discovery_tree_t),
        NULL);

    status =
        mod_ni_710ae_element_init(invalid_element_id, 0, &temp_element_config);

    TEST_ASSERT_EQUAL(FWK_E_PARAM, status);
}

/*!
 * \brief ni710ae unit test: mod_ni710ae_element_init(), memory allocation
 * failure
 *
 *  \details Handle case in mod_ni710ae_element_init() where memory allocation
 * fails
 */
void utest_mod_ni_710ae_element_init_fail_no_memory(void)
{
    int status;

    struct mod_ni_710ae_element_config temp_element_config = {
        .max_number_of_nodes = 1,
        .periphbase_addr = 0xDEDEDEDE,
    };

    fwk_id_t element_id = FWK_ID_ELEMENT(FWK_MODULE_IDX_NI_710AE, 0);
    ni_710ae_ctx.element_count = 1;
    fwk_module_get_element_name_IgnoreAndReturn("Test Element");
    fwk_id_get_element_idx_IgnoreAndReturn(0);

    fwk_mm_alloc_notrap_IgnoreAndReturn(NULL);

    status = mod_ni_710ae_element_init(element_id, 0, &temp_element_config);

    TEST_ASSERT_EQUAL(FWK_E_NOMEM, status);
}

/*!
 * \brief ni710ae unit test: mod_ni710ae_start(), valid config data.
 *
 *  \details Handle case in mod_ni710ae_start() where valid config data
 * is passed.
 */
void utest_mod_ni_710ae_start_success(void)
{
    int status;

    fwk_id_t module_id = FWK_ID_MODULE(FWK_MODULE_IDX_NI_710AE);

    fwk_module_get_element_count_ExpectAndReturn(module_id, NULL, 1);

    struct mod_ni_710ae_element_ctx temp_element_ctx;
    ni_710ae_ctx.element_ctx = &temp_element_ctx;
    ni_710ae_ctx.element_count = 1;
    temp_element_ctx.element_name = "Utest Element";

    fwk_id_get_type_IgnoreAndReturn(FWK_ID_TYPE_MODULE);

    ni710ae_discovery_IgnoreAndReturn(FWK_SUCCESS);

    program_ni_710ae_apu_IgnoreAndReturn(FWK_SUCCESS);

    status = mod_ni_710ae_start(module_id);

    TEST_ASSERT_EQUAL(FWK_SUCCESS, status);
}

/*!
 * \brief ni710ae unit test: mod_ni710ae_start(), valid config data per element.
 *
 *  \details Handle case where a valid element is passed to mod_ni710ae_start(),
 *           and discovery and APU programming succeed.
 */
void utest_mod_ni_710ae_element_start_success(void)
{
    int status;

    fwk_id_t element_id = FWK_ID_ELEMENT(FWK_MODULE_IDX_NI_710AE, 0);

    // Expect type check to identify this is an element
    fwk_id_get_type_ExpectAndReturn(element_id, FWK_ID_TYPE_ELEMENT);

    // Setup context
    static struct mod_ni_710ae_element_config test_element_config = {
        .max_number_of_nodes = 4,
        .periphbase_addr = 0x12340000,
    };

    static struct mod_ni_710ae_element_ctx test_element_ctx = {
        .config = &test_element_config,
        .element_name = "Utest Element",
        .discovery_mem_pool = (void *)0xDEADBEEF, // Preallocated memory pool
        .discovery_mem_pool_index = 0
    };

    ni_710ae_ctx.element_ctx = &test_element_ctx;
    ni_710ae_ctx.element_count = 1;

    fwk_id_get_element_idx_ExpectAndReturn(element_id, 0);
    // fwk_module_get_element_name_ExpectAndReturn(element_id, "Utest Element");

    ni710ae_discovery_IgnoreAndReturn(FWK_SUCCESS);

    program_ni_710ae_apu_IgnoreAndReturn(FWK_SUCCESS);

    status = mod_ni_710ae_start(element_id);

    TEST_ASSERT_EQUAL(FWK_SUCCESS, status);
}
#endif // TESTING_MOD_NI_710AE

#ifdef TESTING_NI_710AE_DISCOVERY_DRV
/*!
 * \brief ni710ae unit test: ni710ae_discovery(), NULL params failure
 *
 *  \details Handle case in ni710ae_discovery() where NULL params passed
 */
void utest_discovery_null_params(void)
{
    int status = ni710ae_discovery(NULL, 0, NULL, NULL, 0);
    TEST_ASSERT_EQUAL(FWK_E_PARAM, status);
}

/*!
 * \brief ni710ae unit test: ni710ae_discovery(), invalid type
 * failure
 *
 *  \details Handle case in ni710ae_discovery() where invalid type passed
 */
void utest_discovery_invalid_type(void)
{
    struct ni710ae_discovery_tree_t node = {
        .type = 0x1234, /* not domain, component, or subfeature */
    };
    struct ni710ae_discovery_tree_t pool[4];
    uint16_t idx = 0;

    int status = ni710ae_discovery(&node, 0x0, pool, &idx, 4);
    TEST_ASSERT_EQUAL(FWK_E_PARAM, status);
}

/*!
 * \brief ni710ae unit test: ni710ae_discovery(), successfull discovery
 *
 *  \details Handle case where PMU type is passed and discovery succeeds
 */
void utest_discovery_component_pmu_returns_success(void)
{
    /* PMU is a component that has no children and returns success early */
    struct ni710ae_discovery_tree_t node = {
        .type = NI710AE_NODE_TYPE_PMU,
    };
    struct ni710ae_discovery_tree_t pool[2];
    uint16_t idx = 0;

    int status = ni710ae_discovery(&node, 0x0, pool, &idx, 2);
    TEST_ASSERT_EQUAL(FWK_SUCCESS, status);
    TEST_ASSERT_EQUAL_UINT32(0, node.children);
    TEST_ASSERT_NULL(node.child);
}

/*!
 * \brief ni710ae unit test: ni710ae_fetch_offset_address(), NULL root
 *
 *  \details Handle case where NULL root is passed to function
 */
void utest_fetch_offset_null_root(void)
{
    uintptr_t addr =
        ni710ae_fetch_offset_address(NULL, NI710AE_NODE_TYPE_PMU, 0, 0);
    TEST_ASSERT_EQUAL_UINTPTR(UINTPTR_MAX, addr);
}

/*!
 * \brief ni710ae unit test: ni710ae_fetch_offset_address(), offset not found
 *
 *  \details Handle case where searched node address couldn't found
 */
void utest_fetch_offset_not_found(void)
{
    struct ni710ae_discovery_tree_t root = { .type = NI710AE_NODE_TYPE_AMNI,
                                             .child = NULL };
    uintptr_t addr = ni710ae_fetch_offset_address(
        &root, NI710AE_NODE_TYPE_AMNI, 9, NI710AE_NODE_TYPE_SUBFEATURE_APU);
    TEST_ASSERT_EQUAL_UINTPTR(UINTPTR_MAX, addr);
}

/*!
 * \brief ni710ae unit test: ni710ae_fetch_offset_address(), successfull fetch
 *
 *  \details Handle case the searched node is the first node and it succeeds
 */
void utest_fetch_offset_immediate_child_match_success(void)
{
    struct ni710ae_discovery_tree_t subfeature = {
        .type = NI710AE_NODE_TYPE_SUBFEATURE_APU,
        .address = 0xA0,
    };
    struct ni710ae_discovery_tree_t component = {
        .type = NI710AE_NODE_TYPE_ASNI,
        .id = 5,
        .child = &subfeature,
    };

    uintptr_t addr = ni710ae_fetch_offset_address(
        &component,
        NI710AE_NODE_TYPE_ASNI,
        5,
        NI710AE_NODE_TYPE_SUBFEATURE_APU);
    TEST_ASSERT_EQUAL_UINTPTR(0xA0, addr);
}

/*!
 * \brief ni710ae unit test: ni710ae_fetch_offset_address(), successfull fetch
 *
 *  \details Handle case the searched node is found after a recursive check
 */
void utest_fetch_offset_recursion_and_siblings_success(void)
{
    /* Build tree:
       root (NI710AE_NODE_TYPE_PMU)
         └─ node_0 (COMPONENT id=1 - NI710AE_NODE_TYPE_ASNI)
             └─ [subfeature_0 @ 0x111]
         └─ node_1 (COMPONENT id=2 - NI710AE_NODE_TYPE_AMNI)
             └─ [subfeature_1 @ 0x222]  <-- target
       siblings: node_1 is first child, node_2 is node_1->sibling
     */
    struct ni710ae_discovery_tree_t subfeature_0 = {
        .type = NI710AE_NODE_TYPE_SUBFEATURE_SAM, .address = 0x111
    };
    struct ni710ae_discovery_tree_t subfeature_1 = {
        .type = NI710AE_NODE_TYPE_SUBFEATURE_APU, .address = 0x222
    };

    struct ni710ae_discovery_tree_t node_1 = { .type = NI710AE_NODE_TYPE_AMNI,
                                               .id = 2,
                                               .child = &subfeature_1 };

    struct ni710ae_discovery_tree_t node_0 = { .type = NI710AE_NODE_TYPE_ASNI,
                                               .id = 1,
                                               .child = &subfeature_0,
                                               .sibling = &node_1 };

    struct ni710ae_discovery_tree_t root = { .type = NI710AE_NODE_TYPE_PMU,
                                             .child = &node_0 };

    uintptr_t addr = ni710ae_fetch_offset_address(
        &root, NI710AE_NODE_TYPE_AMNI, 2, NI710AE_NODE_TYPE_SUBFEATURE_APU);
    TEST_ASSERT_EQUAL_UINTPTR(0x222, addr);
}

#endif // TESTING_NI_710AE_DISCOVERY_DRV

int ni_710ae_test_main(void)
{
    UNITY_BEGIN();

#ifdef TESTING_MOD_NI_710AE
    RUN_TEST(utest_mod_ni_710ae_init_fail_no_element);
    RUN_TEST(utest_mod_ni_710ae_init_fail_no_memory);
    RUN_TEST(utest_mod_ni_710ae_element_init_fail_invalid_index);
    RUN_TEST(utest_mod_ni_710ae_element_init_fail_no_memory);
    RUN_TEST(utest_mod_ni_710ae_init_success);
    RUN_TEST(utest_mod_ni_710ae_start_success);
    RUN_TEST(utest_mod_ni_710ae_element_start_success);
#endif // TESTING_MOD_NI_710AE

#ifdef TESTING_NI_710AE_DISCOVERY_DRV
    RUN_TEST(utest_discovery_null_params);
    RUN_TEST(utest_discovery_invalid_type);
    RUN_TEST(utest_discovery_component_pmu_returns_success);
    RUN_TEST(utest_fetch_offset_null_root);
    RUN_TEST(utest_fetch_offset_not_found);
    RUN_TEST(utest_fetch_offset_immediate_child_match_success);
    RUN_TEST(utest_fetch_offset_recursion_and_siblings_success);
#endif // TESTING_NI_710AE_DISCOVERY_DRV

    return UNITY_END();
}

int main(void)
{
    return ni_710ae_test_main();
}
