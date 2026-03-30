/*
 * Arm SCP/MCP Software
 * Copyright (c) 2024, Arm Limited and Contributors. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <fwk_assert.h>
#include <fwk_mmio.h>
#include <fwk_test.h>

static void test_fwk_mmio_8(void)
{
    uint8_t value;

    fwk_mmio_write_8((uintptr_t)&value, 0x3);
    assert(fwk_mmio_read_8((uintptr_t)&value) == 0x3);

    fwk_mmio_clrbits_8((uintptr_t)&value, 0x2);
    assert(fwk_mmio_read_8((uintptr_t)&value) == 0x1);

    fwk_mmio_setbits_8((uintptr_t)&value, 0x2);
    assert(fwk_mmio_read_8((uintptr_t)&value) == 0x3);

    fwk_mmio_clrsetbits_8((uintptr_t)&value, 0x1, 0x4);
    assert(fwk_mmio_read_8((uintptr_t)&value) == 0x6);
}

static void test_fwk_mmio_16(void)
{
    uint16_t value;

    fwk_mmio_write_16((uintptr_t)&value, 0x3);
    assert(fwk_mmio_read_16((uintptr_t)&value) == 0x3);

    fwk_mmio_clrbits_16((uintptr_t)&value, 0x2);
    assert(fwk_mmio_read_16((uintptr_t)&value) == 0x1);

    fwk_mmio_setbits_16((uintptr_t)&value, 0x2);
    assert(fwk_mmio_read_16((uintptr_t)&value) == 0x3);

    fwk_mmio_clrsetbits_16((uintptr_t)&value, 0x1, 0x4);
    assert(fwk_mmio_read_16((uintptr_t)&value) == 0x6);
}

static void test_fwk_mmio_32(void)
{
    uint32_t value;

    fwk_mmio_write_32((uintptr_t)&value, 0x3);
    assert(fwk_mmio_read_32((uintptr_t)&value) == 0x3);

    fwk_mmio_clrbits_32((uintptr_t)&value, 0x2);
    assert(fwk_mmio_read_32((uintptr_t)&value) == 0x1);

    fwk_mmio_setbits_32((uintptr_t)&value, 0x2);
    assert(fwk_mmio_read_32((uintptr_t)&value) == 0x3);

    fwk_mmio_clrsetbits_32((uintptr_t)&value, 0x1, 0x4);
    assert(fwk_mmio_read_32((uintptr_t)&value) == 0x6);
}

static void test_fwk_mmio_64(void)
{
    uint64_t value;

    fwk_mmio_write_64((uintptr_t)&value, 0x3);
    assert(fwk_mmio_read_64((uintptr_t)&value) == 0x3);

    fwk_mmio_clrbits_64((uintptr_t)&value, 0x2);
    assert(fwk_mmio_read_64((uintptr_t)&value) == 0x1);

    fwk_mmio_setbits_64((uintptr_t)&value, 0x2);
    assert(fwk_mmio_read_64((uintptr_t)&value) == 0x3);

    fwk_mmio_clrsetbits_64((uintptr_t)&value, 0x1, 0x4);
    assert(fwk_mmio_read_64((uintptr_t)&value) == 0x6);
}

static const struct fwk_test_case_desc test_case_table[] = {
    FWK_TEST_CASE(test_fwk_mmio_8),
    FWK_TEST_CASE(test_fwk_mmio_16),
    FWK_TEST_CASE(test_fwk_mmio_32),
    FWK_TEST_CASE(test_fwk_mmio_64),
};

struct fwk_test_suite_desc test_suite = {
    .name = "fwk_mmio",
    .test_case_count = FWK_ARRAY_SIZE(test_case_table),
    .test_case_table = test_case_table,
};
