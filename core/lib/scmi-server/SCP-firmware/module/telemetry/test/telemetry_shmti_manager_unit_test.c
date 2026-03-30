/*
 * Arm SCP/MCP Software
 * Copyright (c) 2025, Arm Limited and Contributors. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include "scp_unity.h"
#include "unity.h"

#include <Mockfwk_mm.h>
#include <Mockfwk_module.h>
#include <telemetry.h>

#include <mod_telemetry.h>

#include UNIT_TEST_SRC

static struct telemetry_shmti_context test_shmti_ctx;
static struct mod_telemetry_shmti_info test_shmti_info;
static uint8_t test_bitmap[16];
static uint8_t mem[1024];

void setUp()
{
    memset(&test_shmti_info, 0, sizeof(test_shmti_info));
    memset(test_bitmap, 0, sizeof(test_bitmap));
    memset(mem, 0, sizeof(mem));
    test_shmti_info.shmti_id = 0;
    test_shmti_info.start_addr = (uintptr_t)mem;
    test_shmti_info.length = 1024;

    test_shmti_ctx.shmti_created = false;
    test_shmti_ctx.allocation_map = test_bitmap;
    test_shmti_ctx.bitmap_len = sizeof(test_bitmap) * 8;
    test_shmti_ctx.shmti_info = &test_shmti_info;
}

void tearDown()
{
}

void test_shmti_bitmap_check_for_free_success(void)
{
    uint32_t bit_start = 0;
    size_t required_bits = 8;
    uint32_t bitmap_len = sizeof(test_bitmap) * 8;

    /* Ensure first 8 bits are free */
    int status = shmti_bitmap_check_for_free(
        test_bitmap,
        required_bits,
        sizeof(test_bitmap) * 8,
        0,
        bitmap_len,
        &bit_start);

    TEST_ASSERT_EQUAL(FWK_SUCCESS, status);
    TEST_ASSERT_EQUAL_UINT32(0, bit_start);
}

void test_shmti_bitmap_check_for_free_no_space(void)
{
    uint32_t bit_start = 0;
    size_t required_bits = 8;
    uint32_t bitmap_len = sizeof(test_bitmap) * 8;

    /* Mark all bits as used */
    memset(test_bitmap, 0xFF, sizeof(test_bitmap));
    int status = shmti_bitmap_check_for_free(
        test_bitmap,
        required_bits,
        sizeof(test_bitmap) * 8,
        0,
        bitmap_len,
        &bit_start);

    TEST_ASSERT_EQUAL(FWK_E_NOMEM, status);
}

void test_shmti_bitmap_check_for_free_middle_space(void)
{
    uint32_t bit_start = 0;
    size_t required_bits = 8;
    uint32_t bitmap_len = sizeof(test_bitmap) * 8;

    /* Mark first and last 8 bits as used */
    test_bitmap[0] = 0xFF;
    test_bitmap[15] = 0xFF;

    int status = shmti_bitmap_check_for_free(
        test_bitmap,
        required_bits,
        sizeof(test_bitmap) * 8,
        0,
        bitmap_len,
        &bit_start);

    TEST_ASSERT_EQUAL(FWK_SUCCESS, status);
    TEST_ASSERT_EQUAL_UINT32(8, bit_start);
}

void test_shmti_bitmap_check_for_free_restricted_search(void)
{
    uint32_t search_bit_start = 16, search_bit_end = 24;
    uint32_t allocated_region_start_bit;
    size_t required_bits = 4;

    /* Ensure range 16-24 is free */
    int status = shmti_bitmap_check_for_free(
        test_bitmap,
        required_bits,
        sizeof(test_bitmap) * 8,
        search_bit_start,
        search_bit_end,
        &allocated_region_start_bit);

    TEST_ASSERT_EQUAL(FWK_SUCCESS, status);
    TEST_ASSERT_EQUAL_UINT32(16, allocated_region_start_bit);
}

void test_shmti_bitmap_check_for_free_restricted_no_space(void)
{
    uint32_t search_bit_start = 16, search_bit_end = 20;
    uint32_t allocated_region_start_bit;
    size_t required_bits = 8;

    /* Search should fail if restricted range is too small */
    int status = shmti_bitmap_check_for_free(
        test_bitmap,
        required_bits,
        sizeof(test_bitmap) * 8,
        search_bit_start,
        search_bit_end,
        &allocated_region_start_bit);

    TEST_ASSERT_EQUAL(FWK_E_NOMEM, status);
}

/* Test: Invalid parameters return FWK_E_PARAM */
void test_shmti_bitmap_check_for_free_invalid_params(void)
{
    uint32_t bit_start = 0;

    /* Null bitmap */
    TEST_ASSERT_EQUAL(
        FWK_E_PARAM,
        shmti_bitmap_check_for_free(
            NULL, 8, sizeof(test_bitmap) * 8, 0, 16, &bit_start));

    /* Null start bit pointer */
    TEST_ASSERT_EQUAL(
        FWK_E_PARAM,
        shmti_bitmap_check_for_free(
            test_bitmap, 8, sizeof(test_bitmap) * 8, 0, 16, NULL));

    /* invalid range for search range. */
    TEST_ASSERT_EQUAL(
        FWK_E_PARAM,
        shmti_bitmap_check_for_free(
            test_bitmap, 8, sizeof(test_bitmap) * 8, 8, 7, &bit_start));

    /* Required bits exceed bitmap size */
    TEST_ASSERT_EQUAL(
        FWK_E_PARAM,
        shmti_bitmap_check_for_free(
            test_bitmap, 2000, sizeof(test_bitmap) * 8, 0, 16, &bit_start));
}

void run_shmti_bitmap_check_for_free_tests(void)
{
    RUN_TEST(test_shmti_bitmap_check_for_free_success);
    RUN_TEST(test_shmti_bitmap_check_for_free_no_space);
    RUN_TEST(test_shmti_bitmap_check_for_free_middle_space);
    RUN_TEST(test_shmti_bitmap_check_for_free_restricted_search);
    RUN_TEST(test_shmti_bitmap_check_for_free_restricted_no_space);
    RUN_TEST(test_shmti_bitmap_check_for_free_invalid_params);
}

void test_shmti_bitmap_mark_used_single_bit(void)
{
    shmti_bitmap_mark_used(test_bitmap, 10, 10, sizeof(test_bitmap) * 8);
    /* Bit 10 -> 0x00000100 */
    TEST_ASSERT_EQUAL_UINT8(0x04, test_bitmap[1]);
    test_bitmap[1] = 0x0;
}

void test_shmti_bitmap_mark_used_same_byte(void)
{
    shmti_bitmap_mark_used(test_bitmap, 3, 6, sizeof(test_bitmap) * 8);
    /* Bits 3-6 -> 01111000 */
    TEST_ASSERT_EQUAL_UINT8(0x78, test_bitmap[0]);
    test_bitmap[0] = 0x0;
}

void test_shmti_bitmap_mark_used_multiple_bytes(void)
{
    shmti_bitmap_mark_used(test_bitmap, 5, 11, sizeof(test_bitmap) * 8);
    /* Bits 5-7 -> 11100000 */
    TEST_ASSERT_EQUAL_UINT8(0xE0, test_bitmap[0]);
    /* Bits 8-11 -> 00001111 */
    TEST_ASSERT_EQUAL_UINT8(0x0F, test_bitmap[1]);
}

void test_shmti_bitmap_mark_used_full_byte(void)
{
    shmti_bitmap_mark_used(test_bitmap, 8, 15, sizeof(test_bitmap) * 8);
    /* Bits 8-15 -> 11111111 */
    TEST_ASSERT_EQUAL_UINT8(0xFF, test_bitmap[1]);
}

void test_shmti_bitmap_mark_used_boundary(void)
{
    shmti_bitmap_mark_used(test_bitmap, 120, 127, sizeof(test_bitmap) * 8);
    /* Last byte fully set */
    TEST_ASSERT_EQUAL_UINT8(0xFF, test_bitmap[15]);
}

void test_shmti_bitmap_mark_used_null_bitmap(void)
{
    size_t i;
    shmti_bitmap_mark_used(NULL, 5, 10, sizeof(test_bitmap) * 8);
    /* Expect no changes */
    for (i = 0; i < sizeof(test_bitmap); i++) {
        TEST_ASSERT_EQUAL_UINT8(0x00, test_bitmap[i]);
    }
}

void test_shmti_bitmap_mark_used_invalid_range(void)
{
    size_t i;
    shmti_bitmap_mark_used(test_bitmap, 10, 5, sizeof(test_bitmap) * 8);
    /* Expect no changes */
    for (i = 0; i < sizeof(test_bitmap); i++) {
        TEST_ASSERT_EQUAL_UINT8(0x00, test_bitmap[i]);
    }
}

void test_shmti_bitmap_mark_used_out_of_bounds(void)
{
    size_t i;
    shmti_bitmap_mark_used(test_bitmap, 120, 1300, sizeof(test_bitmap) * 8);
    /* Expect no changes */
    for (i = 0; i < sizeof(test_bitmap); i++) {
        TEST_ASSERT_EQUAL_UINT8(0x00, test_bitmap[i]);
    }
}

void test_shmti_bitmap_mark_free_single_bit(void)
{
    test_bitmap[1] = 0xFF;
    shmti_bitmap_mark_free(test_bitmap, 10, 10, sizeof(test_bitmap) * 8);
    /* Bit 10 -> 0x00000100 cleared */
    TEST_ASSERT_EQUAL_UINT8(0xFB, test_bitmap[1]);
}

void test_shmti_bitmap_mark_free_same_byte(void)
{
    test_bitmap[0] = 0xFF;
    shmti_bitmap_mark_free(test_bitmap, 3, 6, sizeof(test_bitmap) * 8);
    /* Bits 3-6 -> 01111000 cleared */
    TEST_ASSERT_EQUAL_UINT8(0x87, test_bitmap[0]);
}

void test_shmti_bitmap_mark_free_multiple_bytes(void)
{
    test_bitmap[0] = test_bitmap[1] = 0xFF;
    shmti_bitmap_mark_free(test_bitmap, 5, 11, sizeof(test_bitmap) * 8);
    /* Bits 5-7 cleared */
    TEST_ASSERT_EQUAL_UINT8(0x1F, test_bitmap[0]);
    /* Bits 8-11 cleared */
    TEST_ASSERT_EQUAL_UINT8(0xF0, test_bitmap[1]);
}

void test_shmti_bitmap_mark_free_full_byte(void)
{
    shmti_bitmap_mark_free(test_bitmap, 8, 15, sizeof(test_bitmap) * 8);
    /* Entire byte cleared */
    TEST_ASSERT_EQUAL_UINT8(0x00, test_bitmap[1]);
}

void test_shmti_bitmap_mark_free_boundary(void)
{
    shmti_bitmap_mark_free(test_bitmap, 120, 127, sizeof(test_bitmap) * 8);
    /* Last byte cleared */
    TEST_ASSERT_EQUAL_UINT8(0x00, test_bitmap[15]);
}

void test_shmti_bitmap_mark_free_null_bitmap(void)
{
    size_t i;
    memset(test_bitmap, 0xFF, sizeof(test_bitmap));
    shmti_bitmap_mark_free(NULL, 5, 10, sizeof(test_bitmap) * 8);
    for (i = 0; i < sizeof(test_bitmap); i++) {
        TEST_ASSERT_EQUAL_UINT8(0xFF, test_bitmap[i]);
    }
}

void test_shmti_bitmap_mark_free_invalid_range(void)
{
    size_t i;
    memset(test_bitmap, 0xFF, sizeof(test_bitmap));
    shmti_bitmap_mark_free(test_bitmap, 10, 5, sizeof(test_bitmap) * 8);
    /* Expect no changes */
    for (i = 0; i < sizeof(test_bitmap); i++) {
        TEST_ASSERT_EQUAL_UINT8(0xFF, test_bitmap[i]);
    }
}

void test_shmti_bitmap_mark_free_out_of_bounds(void)
{
    size_t i;
    memset(test_bitmap, 0xFF, sizeof(test_bitmap));
    shmti_bitmap_mark_free(test_bitmap, 120, 1300, sizeof(test_bitmap) * 8);
    /* Expect no changes */
    for (i = 0; i < sizeof(test_bitmap); i++) {
        TEST_ASSERT_EQUAL_UINT8(0xFF, test_bitmap[i]);
    }
}

void run_shmti_bitmap_mark_free_tests(void)
{
    RUN_TEST(test_shmti_bitmap_mark_free_single_bit);
    RUN_TEST(test_shmti_bitmap_mark_free_same_byte);
    RUN_TEST(test_shmti_bitmap_mark_free_multiple_bytes);
    RUN_TEST(test_shmti_bitmap_mark_free_full_byte);
    RUN_TEST(test_shmti_bitmap_mark_free_boundary);
    RUN_TEST(test_shmti_bitmap_mark_free_null_bitmap);
    RUN_TEST(test_shmti_bitmap_mark_free_invalid_range);
    RUN_TEST(test_shmti_bitmap_mark_free_out_of_bounds);
}

void run_shmti_bitmap_mark_used_tests(void)
{
    RUN_TEST(test_shmti_bitmap_mark_used_single_bit);
    RUN_TEST(test_shmti_bitmap_mark_used_same_byte);
    RUN_TEST(test_shmti_bitmap_mark_used_multiple_bytes);
    RUN_TEST(test_shmti_bitmap_mark_used_full_byte);
    RUN_TEST(test_shmti_bitmap_mark_used_boundary);
    RUN_TEST(test_shmti_bitmap_mark_used_null_bitmap);
    RUN_TEST(test_shmti_bitmap_mark_used_invalid_range);
    RUN_TEST(test_shmti_bitmap_mark_used_out_of_bounds);
}

void test_shmti_init_bitmap_success(void)
{
    size_t test_mem_size = 1024;
    uint8_t *test_alloced_bitmap = NULL;
    uint32_t test_alloced_bitmap_len = 0;

    memset(test_bitmap, 0, sizeof(test_bitmap));
    fwk_mm_alloc_ExpectAnyArgsAndReturn(test_bitmap);

    int status = shmti_init_bitmap(
        &test_alloced_bitmap, test_mem_size, &test_alloced_bitmap_len);

    TEST_ASSERT_EQUAL(FWK_SUCCESS, status);
    TEST_ASSERT_EQUAL_UINT32(test_mem_size / 8, test_alloced_bitmap_len);
    TEST_ASSERT_NOT_NULL(test_alloced_bitmap);
}

void test_shmti_init_bitmap_null_bitmap(void)
{
    size_t test_mem_size = 1024;
    uint32_t test_alloced_bitmap_len = 0;

    int status =
        shmti_init_bitmap(NULL, test_mem_size, &test_alloced_bitmap_len);

    TEST_ASSERT_EQUAL(FWK_E_PARAM, status);
}

void test_shmti_init_bitmap_null_bitmap_len(void)
{
    size_t test_mem_size = 1024;
    uint8_t *test_alloced_bitmap = NULL;

    int status = shmti_init_bitmap(&test_alloced_bitmap, test_mem_size, NULL);

    TEST_ASSERT_EQUAL(FWK_E_PARAM, status);
}

void test_shmti_init_bitmap_zero_mem_size(void)
{
    uint8_t *test_alloced_bitmap = test_bitmap;
    uint32_t test_alloced_bitmap_len = 0;
    int status =
        shmti_init_bitmap(&test_alloced_bitmap, 0, &test_alloced_bitmap_len);

    TEST_ASSERT_EQUAL(FWK_E_PARAM, status);
}

void test_shmti_init_bitmap_allocation_failure(void)
{
    size_t test_mem_size = 1024;
    uint8_t *test_alloced_bitmap = NULL;
    uint32_t test_alloced_bitmap_len = 0;
    ;

    fwk_mm_alloc_ExpectAnyArgsAndReturn(NULL);

    int status = shmti_init_bitmap(
        &test_alloced_bitmap, test_mem_size, &test_alloced_bitmap_len);

    TEST_ASSERT_EQUAL(FWK_E_NOMEM, status);
    TEST_ASSERT_NULL(test_alloced_bitmap);
}

void test_shmti_init_bitmap_zero_initialized(void)
{
    uint8_t *test_alloced_bitmap = NULL;
    uint32_t test_alloced_bitmap_len = 0;
    size_t test_mem_size = 1028;
    /* Fill with non-zero values */
    memset(test_bitmap, 0xFF, sizeof(test_bitmap));
    fwk_mm_alloc_ExpectAnyArgsAndReturn(test_bitmap);

    int status = shmti_init_bitmap(
        &test_alloced_bitmap, test_mem_size, &test_alloced_bitmap_len);

    TEST_ASSERT_EQUAL(FWK_SUCCESS, status);
    TEST_ASSERT_EQUAL_UINT32(test_mem_size / 8, test_alloced_bitmap_len);
    TEST_ASSERT_NOT_NULL(test_bitmap);

    /* Ensure all bytes are zero */
    for (size_t i = 0; i < test_alloced_bitmap_len; i++) {
        TEST_ASSERT_EQUAL_UINT8(0x00, test_alloced_bitmap[i]);
    }
}

void run_shmti_init_bitmap_tests(void)
{
    RUN_TEST(test_shmti_init_bitmap_success);
    RUN_TEST(test_shmti_init_bitmap_null_bitmap);
    RUN_TEST(test_shmti_init_bitmap_null_bitmap_len);
    RUN_TEST(test_shmti_init_bitmap_zero_mem_size);
    RUN_TEST(test_shmti_init_bitmap_allocation_failure);
    RUN_TEST(test_shmti_init_bitmap_zero_initialized);
}

void test_shmti_create_success(void)
{
    int status;
    memset(test_bitmap, 0, sizeof(test_bitmap));
    fwk_mm_alloc_ExpectAnyArgsAndReturn(test_bitmap);
    uint32_t expected_bit_map_sz = BITMAP_GET_MIN_BYTES_FOR_BIT_LEN(
        BITMAP_GET_MIN_BITS_FOR_SIZE(test_shmti_info.length));
    uintptr_t expected_end_seq_addr =
        (uintptr_t)(mem + test_shmti_info.length - SHMTI_MATCH_SEQUENCE_SIZE);
    /* First two bits for prologue: 00000011*/
    uint32_t expected_start_of_allocation_map_for_prologue = 0x3;
    /* Last one bit for Epilogue: 10000000*/
    uint32_t expected_end_of_allocation_map_for_epilogue = 0x80;

    status = shmti_create(&test_shmti_ctx);
    TEST_ASSERT_EQUAL(FWK_SUCCESS, status);
    TEST_ASSERT_TRUE(test_shmti_ctx.shmti_created);
    TEST_ASSERT_EQUAL(
        test_shmti_ctx.bitmap_len,
        BITMAP_GET_MIN_BITS_FOR_SIZE(test_shmti_info.length));
    TEST_ASSERT_EQUAL(expected_end_seq_addr, test_shmti_ctx.end_seq_addr);
    /* Check if Prologue has been updated. */
    TEST_ASSERT_EQUAL_HEX8(
        expected_start_of_allocation_map_for_prologue,
        test_shmti_ctx.allocation_map[0]);
    /* Check if Epilogue has been updated. */
    TEST_ASSERT_EQUAL_HEX8(
        expected_end_of_allocation_map_for_epilogue,
        test_shmti_ctx.allocation_map[expected_bit_map_sz - 1]);
    /* Check if everything else remains available. */
    TEST_ASSERT_EACH_EQUAL_HEX8(
        0x0, &test_shmti_ctx.allocation_map[1], expected_bit_map_sz - 2);
}

void test_shmti_create_null_context(void)
{
    int status = shmti_create(NULL);
    TEST_ASSERT_EQUAL(FWK_E_PARAM, status);
}

void test_shmti_create_null_shmti_info(void)
{
    test_shmti_ctx.shmti_info = NULL;
    int status = shmti_create(&test_shmti_ctx);
    TEST_ASSERT_EQUAL(FWK_E_PARAM, status);
}

void test_shmti_create_allocation_failure(void)
{
    fwk_mm_alloc_ExpectAnyArgsAndReturn(NULL);

    int status = shmti_create(&test_shmti_ctx);

    TEST_ASSERT_EQUAL(FWK_E_NOMEM, status);
    TEST_ASSERT_FALSE(test_shmti_ctx.shmti_created);
}

void test_shmti_create_bitmap_initialization(void)
{
    memset(test_bitmap, 0, sizeof(test_bitmap));
    fwk_mm_alloc_ExpectAnyArgsAndReturn(test_bitmap);

    int status = shmti_create(&test_shmti_ctx);

    TEST_ASSERT_EQUAL(FWK_SUCCESS, status);
    TEST_ASSERT_EQUAL(128, test_shmti_ctx.bitmap_len);
}

void test_shmti_create_header_metadata_written(void)
{
    memset(test_bitmap, 0, sizeof(test_bitmap));
    fwk_mm_alloc_ExpectAnyArgsAndReturn(test_bitmap);
    int status = shmti_create(&test_shmti_ctx);

    TEST_ASSERT_EQUAL(FWK_SUCCESS, status);
    TEST_ASSERT_NOT_EQUAL(0, test_shmti_ctx.end_seq_addr);
}

void test_shmti_create_reserved_memory_marked_used(void)
{
    memset(test_bitmap, 0, sizeof(test_bitmap));
    fwk_mm_alloc_ExpectAnyArgsAndReturn(test_bitmap);

    int status = shmti_create(&test_shmti_ctx);

    TEST_ASSERT_EQUAL(FWK_SUCCESS, status);
    TEST_ASSERT_NOT_EQUAL(0, test_shmti_ctx.allocation_map[0]);
}

void run_shmti_create_tests(void)
{
    RUN_TEST(test_shmti_create_success);
    RUN_TEST(test_shmti_create_null_context);
    RUN_TEST(test_shmti_create_null_shmti_info);
    RUN_TEST(test_shmti_create_allocation_failure);
    RUN_TEST(test_shmti_create_bitmap_initialization);
    RUN_TEST(test_shmti_create_header_metadata_written);
    RUN_TEST(test_shmti_create_reserved_memory_marked_used);
}

void test_shmti_alloc_pool_success(void)
{
    uint32_t arg_offset;
    memset(&test_shmti_ctx, 0, sizeof(struct telemetry_shmti_context));
    memset(&test_shmti_info, 0, sizeof(struct mod_telemetry_shmti_info));
    test_shmti_info.shmti_id = 1;
    test_shmti_info.length = 1024;
    test_shmti_ctx.shmti_info = &test_shmti_info;
    test_shmti_ctx.shmti_created = true;
    test_shmti_ctx.bitmap_len = 1024 / 8;
    test_shmti_ctx.allocation_map = test_bitmap;

    /* Make continuous allocations of 16 bytes. */
    for (size_t sz = 16; sz <= 1024; sz += 16) {
        TEST_ASSERT_EQUAL(
            FWK_SUCCESS, shmti_alloc_pool(&test_shmti_ctx, 16, &arg_offset));
        TEST_ASSERT_EQUAL(sz - 16, arg_offset);
    }
}

void test_shmti_alloc_pool_null_offset_arg_failure(void)
{
    TEST_ASSERT_EQUAL(FWK_E_PARAM, shmti_alloc_pool(&test_shmti_ctx, 16, NULL));
}

void test_shmti_alloc_pool_non_initialised_shmti_failure(void)
{
    uint32_t arg_offset;
    memset(&test_shmti_ctx, 0, sizeof(struct telemetry_shmti_context));
    memset(&test_shmti_info, 0, sizeof(struct mod_telemetry_shmti_info));
    test_shmti_ctx.shmti_created = false;
    TEST_ASSERT_EQUAL(
        FWK_E_STATE, shmti_alloc_pool(&test_shmti_ctx, 16, &arg_offset));
}

void test_shmti_alloc_pool_no_space_returns_enomem_failure(void)
{
    uint32_t arg_offset;
    memset(&test_shmti_ctx, 0, sizeof(struct telemetry_shmti_context));
    memset(&test_shmti_info, 0, sizeof(struct mod_telemetry_shmti_info));
    test_shmti_info.shmti_id = 1;
    test_shmti_info.length = 1024;
    test_shmti_ctx.shmti_info = &test_shmti_info;
    test_shmti_ctx.shmti_created = true;
    test_shmti_ctx.bitmap_len = 1024 / 8;
    test_shmti_ctx.allocation_map = test_bitmap;
    memset(test_bitmap, UINT32_C(0xFF), sizeof(test_bitmap));

    TEST_ASSERT_EQUAL(
        FWK_E_NOMEM, shmti_alloc_pool(&test_shmti_ctx, 16, &arg_offset));

    test_bitmap[15] = 0xF;
    TEST_ASSERT_EQUAL(
        FWK_SUCCESS, shmti_alloc_pool(&test_shmti_ctx, 16, &arg_offset));
}

void run_shmti_alloc_tests(void)
{
    RUN_TEST(test_shmti_alloc_pool_success);
    RUN_TEST(test_shmti_alloc_pool_null_offset_arg_failure);
    RUN_TEST(test_shmti_alloc_pool_non_initialised_shmti_failure);
    RUN_TEST(test_shmti_alloc_pool_no_space_returns_enomem_failure);
}

void test_shmti_free_pool_success(void)
{
    uint32_t arg_offset;
    memset(&test_shmti_ctx, 0, sizeof(struct telemetry_shmti_context));
    memset(&test_shmti_info, 0, sizeof(struct mod_telemetry_shmti_info));
    memset(test_bitmap, 0, sizeof(test_bitmap));
    test_shmti_info.shmti_id = 1;
    test_shmti_info.length = 1024;
    test_shmti_ctx.shmti_info = &test_shmti_info;
    test_shmti_ctx.shmti_created = true;
    test_shmti_ctx.bitmap_len = 128;
    test_shmti_ctx.allocation_map = test_bitmap;

    for (size_t sz = 16; sz <= 1024; sz += 16) {
        TEST_ASSERT_EQUAL(
            FWK_SUCCESS, shmti_alloc_pool(&test_shmti_ctx, 16, &arg_offset));
        TEST_ASSERT_EQUAL(sz - 16, arg_offset);
    }

    for (size_t offset = 0; offset < 1024; offset += 16) {
        TEST_ASSERT_EQUAL(
            FWK_SUCCESS, shmti_free_pool(&test_shmti_ctx, 16, offset));
    }
}

void test_shmti_free_pool_size_zero(void)
{
    test_shmti_ctx.shmti_created = true;
    TEST_ASSERT_EQUAL(FWK_E_PARAM, shmti_free_pool(&test_shmti_ctx, 0, 16));
}

void test_shmti_free_pool_invalid_offset(void)
{
    test_shmti_ctx.shmti_created = true;
    TEST_ASSERT_EQUAL(
        FWK_E_PARAM,
        shmti_free_pool(&test_shmti_ctx, 16, test_shmti_info.length + 1));
}

void test_shmti_free_pool_uninitialized_shmti(void)
{
    test_shmti_ctx.shmti_created = false;
    TEST_ASSERT_EQUAL(FWK_E_STATE, shmti_free_pool(&test_shmti_ctx, 16, 16));
}

void run_shmti_free_tests(void)
{
    RUN_TEST(test_shmti_free_pool_success);
    RUN_TEST(test_shmti_free_pool_size_zero);
    RUN_TEST(test_shmti_free_pool_invalid_offset);
    RUN_TEST(test_shmti_free_pool_uninitialized_shmti);
}

void test_shmti_seq_inc_success(void)
{
    int status;
    uint64_t start_value = 10;
    uint64_t end_value = 20;
    test_shmti_info.start_addr = (uintptr_t)&start_value;
    test_shmti_ctx.end_seq_addr = (uintptr_t)&end_value;

    status = shmti_seq_inc(&test_shmti_ctx);
    TEST_ASSERT_EQUAL(FWK_SUCCESS, status);
    TEST_ASSERT_EQUAL_UINT64(11, start_value);
    TEST_ASSERT_EQUAL_UINT64(21, end_value);
}

void test_shmti_seq_inc_null_start_or_end_addr(void)
{
    int status;
    uint64_t start_value = 10;
    uint64_t end_value = 20;
    test_shmti_info.start_addr = (uintptr_t)&start_value;
    test_shmti_ctx.end_seq_addr = (uintptr_t)&end_value;

    /* Inject failure on start match sequence addr. */
    test_shmti_info.start_addr = (uintptr_t)NULL;
    status = shmti_seq_inc(&test_shmti_ctx);
    TEST_ASSERT_EQUAL(FWK_E_PARAM, status);
    TEST_ASSERT_EQUAL_UINT64(10, start_value);
    TEST_ASSERT_EQUAL_UINT64(20, end_value);

    test_shmti_info.start_addr = (uintptr_t)&start_value;
    /* Inject failure on end match sequence addr. */
    test_shmti_ctx.end_seq_addr = (uintptr_t)NULL;
    status = shmti_seq_inc(&test_shmti_ctx);
    TEST_ASSERT_EQUAL(FWK_E_PARAM, status);
    TEST_ASSERT_EQUAL_UINT64(10, start_value);
    TEST_ASSERT_EQUAL_UINT64(20, end_value);
}

/* Helper function to invoke all shmti_seq_inc tests */
void run_shmti_seq_inc_tests(void)
{
    RUN_TEST(test_shmti_seq_inc_success);
    RUN_TEST(test_shmti_seq_inc_null_start_or_end_addr);
}

int main(void)
{
    UNITY_BEGIN();

    run_shmti_bitmap_check_for_free_tests();
    run_shmti_bitmap_mark_used_tests();
    run_shmti_bitmap_mark_free_tests();
    run_shmti_init_bitmap_tests();
    run_shmti_create_tests();
    run_shmti_alloc_tests();
    run_shmti_free_tests();
    run_shmti_seq_inc_tests();

    return UNITY_END();
}
