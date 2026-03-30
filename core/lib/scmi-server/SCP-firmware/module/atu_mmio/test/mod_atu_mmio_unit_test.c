/*
 * Arm SCP/MCP Software
 * Copyright (c) 2025, Arm Limited and Contributors. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include "internal/Mockfwk_id_internal.h"
#include "scp_unity.h"
#include "unity.h"

#include <Mockfwk_id.h>
#include <Mockfwk_module.h>
#include <Mockmod_atu_mmio_extra.h>

#include <mod_atu.h>
#include <mod_atu_mmio.h>

#include <fwk_module_idx.h>

#include <string.h>

#include UNIT_TEST_SRC

/* Map size-aligned base physical address*/
#define ATU_MMIO_ALIGNED_PHY_ADDR 0x18000000

/* Map size */
#define ATU_MMIO_MAP_SIZE (1 * FWK_MIB)
#define ATU_MMIO_ALIGNED_PHY_ADDR_END \
    (ATU_MMIO_ALIGNED_PHY_ADDR + ATU_MMIO_MAP_SIZE - 1)

/* Edge case when 0x0 is base address*/
#define ATU_MMIO_ZERO_PHY_ADDR 0x00000000
#define ATU_MMIO_ZERO_PHY_ADDR_END \
    (ATU_MMIO_ZERO_PHY_ADDR + ATU_MMIO_MAP_SIZE - 1)

/* Non-aligned base physical address */
#define ATU_MMIO_NON_ALIGNED_PHY_ADDR 0x18001000
#define ATU_MMIO_ADDR_OFFSET          (4 * FWK_KIB)

/* Logical address base*/
#define ATU_MMIO_FAKE_LOGICAL_ADDR 0x20000000

static struct mod_atu_mmio_config atu_mmio_config = {
    .window_address = ATU_MMIO_FAKE_LOGICAL_ADDR,
    .map_size = ATU_MMIO_MAP_SIZE,
    .atu_id = FWK_ID_ELEMENT_INIT(FWK_MODULE_IDX_ATU, 0),
    .atu_api_id = FWK_ID_API_INIT(FWK_MODULE_IDX_ATU, MOD_ATU_API_IDX_ATU),
};

struct mod_atu_api mock_mod_atu_api = {
    .add_region = mock_atu_add_region,
    .remove_region = mock_atu_remove_region,
};

void setUp(void)
{
    memset(&ctx, 0, sizeof(ctx));
    ctx.config = &atu_mmio_config;
    ctx.atu_api = &mock_mod_atu_api;
}

void tearDown(void)
{
    /* Do Nothing */
}

/*!
 * \brief ATU MMIO unit test:
 * test_mod_atu_mmio_process_bind_request_invalid_param()
 *
 * \details Handle case in ATU MMIO process bind request where an invalid
 *      API ID is passed.
 */
void test_mod_atu_mmio_process_bind_request_invalid_param(void)
{
    int status;
    struct mod_atu_mmio_api *api;
    api = NULL;
    fwk_id_t mod_id = FWK_ID_MODULE_INIT(FWK_MODULE_IDX_ATU_MMIO);

    /* Use Invalid API Index*/
    fwk_id_t api_id =
        FWK_ID_API(FWK_MODULE_IDX_ATU_MMIO, MOD_ATU_MMIO_API_IDX_MAX);
    fwk_id_t source_id = FWK_ID_MODULE_INIT(FWK_MODULE_IDX_FAKE);

    fwk_module_is_valid_module_id_ExpectAndReturn(mod_id, true);
    fwk_id_get_api_idx_ExpectAndReturn(api_id, MOD_ATU_MMIO_API_IDX_MAX);

    status = atu_mmio_process_bind_request(
        source_id, mod_id, api_id, (const void **)&api);

    TEST_ASSERT_EQUAL(FWK_E_PARAM, status);
    TEST_ASSERT_NULL(api);
}

/*!
 * \brief ATU MMIO unit test: test_mod_atu_mmio_process_bind_request_valid()
 *
 * \details Handle case in ATU MMIO process bind request where a valid
 * parameters are passed.
 */
void test_mod_atu_mmio_process_bind_request_valid(void)
{
    int status;
    struct mod_atu_mmio_api *api;
    api = NULL;
    fwk_id_t mod_id = FWK_ID_MODULE_INIT(FWK_MODULE_IDX_ATU_MMIO);
    fwk_id_t api_id =
        FWK_ID_API(FWK_MODULE_IDX_ATU_MMIO, MOD_ATU_MMIO_API_IDX_MEM_RW);
    fwk_id_t source_id = FWK_ID_MODULE_INIT(FWK_MODULE_IDX_FAKE);

    fwk_module_is_valid_module_id_ExpectAndReturn(mod_id, true);
    fwk_id_get_api_idx_ExpectAndReturn(api_id, MOD_ATU_MMIO_API_IDX_MEM_RW);

    status = atu_mmio_process_bind_request(
        source_id, mod_id, api_id, (const void **)&api);

    TEST_ASSERT_EQUAL(FWK_SUCCESS, status);
    TEST_ASSERT_NOT_NULL(api);
    TEST_ASSERT_EQUAL_PTR(&rw_api, api);
}

/*!
 * \brief ATU MMIO unit test: test_mod_atu_mmio_zero_phy_address()
 *
 * \details Handle case where physical address is zero.
 */
void test_mod_atu_mmio_zero_phy_address(void)
{
    uint64_t phy_address = ATU_MMIO_ZERO_PHY_ADDR;
    uintptr_t logical_address;

    mock_atu_remove_region_ExpectAnyArgsAndReturn(FWK_SUCCESS);
    mock_atu_add_region_ExpectAnyArgsAndReturn(FWK_SUCCESS);
    logical_address = map_region(phy_address, ATU_MMIO_RW_WIDTH_BYTE);
    TEST_ASSERT_EQUAL(ATU_MMIO_FAKE_LOGICAL_ADDR, logical_address);

    /* Iterate over map region */
    for (uint64_t addr = phy_address; addr < ATU_MMIO_ZERO_PHY_ADDR_END;
         addr += ATU_MMIO_ADDR_OFFSET) {
        logical_address = map_region(addr, ATU_MMIO_RW_WIDTH_BYTE);
        TEST_ASSERT_EQUAL(
            ATU_MMIO_FAKE_LOGICAL_ADDR + (addr - ATU_MMIO_ZERO_PHY_ADDR),
            logical_address);
    }
    /* Over the map size, map new region and return base logical address */
    mock_atu_remove_region_ExpectAnyArgsAndReturn(FWK_SUCCESS);
    mock_atu_add_region_ExpectAnyArgsAndReturn(FWK_SUCCESS);
    logical_address = map_region(
        ATU_MMIO_ZERO_PHY_ADDR + ATU_MMIO_MAP_SIZE, ATU_MMIO_RW_WIDTH_BYTE);
    TEST_ASSERT_EQUAL(ATU_MMIO_FAKE_LOGICAL_ADDR, logical_address);
}

/*!
 * \brief ATU MMIO unit test: test_mod_atu_mmio_aligned_phy_address()
 *
 * \details Handle case where physical address is aligned.
 */
void test_mod_atu_mmio_aligned_phy_address(void)
{
    uint64_t phy_address = ATU_MMIO_ALIGNED_PHY_ADDR;
    uintptr_t logical_address;

    mock_atu_remove_region_ExpectAnyArgsAndReturn(FWK_SUCCESS);
    mock_atu_add_region_ExpectAnyArgsAndReturn(FWK_SUCCESS);
    logical_address = map_region(phy_address, ATU_MMIO_RW_WIDTH_BYTE);
    TEST_ASSERT_EQUAL(ATU_MMIO_FAKE_LOGICAL_ADDR, logical_address);

    /* Iterate over map region */
    for (uint64_t addr = phy_address; addr < ATU_MMIO_ALIGNED_PHY_ADDR_END;
         addr += ATU_MMIO_ADDR_OFFSET) {
        logical_address = map_region(addr, ATU_MMIO_RW_WIDTH_BYTE);
        TEST_ASSERT_EQUAL(
            ATU_MMIO_FAKE_LOGICAL_ADDR + (addr - ATU_MMIO_ALIGNED_PHY_ADDR),
            logical_address);
    }
    /* Over the map size, map new region and return base logical address */
    mock_atu_remove_region_ExpectAnyArgsAndReturn(FWK_SUCCESS);
    mock_atu_add_region_ExpectAnyArgsAndReturn(FWK_SUCCESS);
    logical_address = map_region(
        ATU_MMIO_ALIGNED_PHY_ADDR + ATU_MMIO_MAP_SIZE, ATU_MMIO_RW_WIDTH_BYTE);
    TEST_ASSERT_EQUAL(ATU_MMIO_FAKE_LOGICAL_ADDR, logical_address);
}

/*!
 * \brief ATU MMIO unit test: test_mod_atu_mmio_non_aligned_phy_address()
 *
 * \details Handle case where physical address is not aligned. Map aligned
 *      address and return logical address with offset.
 */
void test_mod_atu_mmio_non_aligned_phy_address(void)
{
    uint64_t phy_address = ATU_MMIO_NON_ALIGNED_PHY_ADDR;
    uintptr_t logical_address;

    mock_atu_remove_region_ExpectAnyArgsAndReturn(FWK_SUCCESS);
    mock_atu_add_region_ExpectAnyArgsAndReturn(FWK_SUCCESS);
    logical_address = map_region(phy_address, ATU_MMIO_RW_WIDTH_BYTE);
    TEST_ASSERT_EQUAL(
        ATU_MMIO_FAKE_LOGICAL_ADDR + (phy_address - ATU_MMIO_ALIGNED_PHY_ADDR),
        logical_address);
}

int atu_mmio_test_main(void)
{
    UNITY_BEGIN();

    RUN_TEST(test_mod_atu_mmio_process_bind_request_invalid_param);
    RUN_TEST(test_mod_atu_mmio_process_bind_request_valid);
    RUN_TEST(test_mod_atu_mmio_zero_phy_address);
    RUN_TEST(test_mod_atu_mmio_aligned_phy_address);
    RUN_TEST(test_mod_atu_mmio_non_aligned_phy_address);

    return UNITY_END();
}

int main(void)
{
    return atu_mmio_test_main();
}
