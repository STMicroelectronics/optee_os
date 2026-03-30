/*
 * Arm SCP/MCP Software
 * Copyright (c) 2024-2025, Arm Limited and Contributors. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include "internal/gicx00_reg.h"
#include "scp_unity.h"
#include "unity.h"

#include <Mockfwk_id.h>
#include <Mockgicx00_reg.h>
#include <config_gicx00.h>

#include <string.h>

#include UNIT_TEST_SRC

#define FWK_MODULE_IDX_GICX00 0

void setUp(void)
{
    memset(gicd_reg, 0, GICD_SIZE);
    memset(gicr_reg, 0, GICR_SIZE);
}

void tearDown(void)
{
}

void test_mod_gicx00_init_gic720ae(void)
{
    fwk_id_t id;
    int status;
    uint32_t reg;

    id = FWK_ID_MODULE(FWK_MODULE_IDX_GICX00);

    write_icc_sre_Expect(0xF);
    write_icc_pmr_Expect(0xFF);
    write_igrpen0_el1_Expect(0x1);
    read_mpidr_el1_ExpectAndReturn(0x7);

    /* Configure as a GIC720AE and initialize the GICR_PWRR register */
    fwk_mmio_write_32((uintptr_t)(gicr_reg + GICR_IIDR), GICR_IIDR_GIC720AE);
    fwk_mmio_write_32((uintptr_t)(gicr_reg + GICR_PWRR), GICR_PWRR_RDPD);

    status = gicx00_init(id, 0, config_gicx00_ut.data);
    TEST_ASSERT_EQUAL(FWK_SUCCESS, status);

    /* Verify that PWRR was cleared */
    reg = fwk_mmio_read_32((uintptr_t)(gicr_reg + GICR_PWRR));
    TEST_ASSERT_EQUAL(0, reg);
}

void test_mod_gicx00_init(void)
{
    fwk_id_t id;
    int status;
    uint32_t reg;
    uint64_t irouter;

    id = FWK_ID_MODULE(FWK_MODULE_IDX_GICX00);

    write_icc_sre_Expect(0xF);
    write_icc_pmr_Expect(0xFF);
    write_igrpen0_el1_Expect(0x1);
    read_mpidr_el1_ExpectAndReturn(0x7);

    /* Initialize GICR_WAKER */
    fwk_mmio_write_32(
        (uintptr_t)(gicr_reg + GICR_WAKER), GICR_WAKER_PROCESSOR_SLEEP);

    /* Initialize GICR_TYPER with 1020 interrupts */
    fwk_mmio_write_32((uintptr_t)(gicd_reg + GICD_TYPER), 0x1E);

    status = gicx00_init(id, 0, config_gicx00_ut.data);
    TEST_ASSERT_EQUAL(FWK_SUCCESS, status);

    /* Verify GIC registers */
    reg = fwk_mmio_read_32((uintptr_t)(gicd_reg + GICD_CTLR));
    TEST_ASSERT_EQUAL(0x51, reg);
    reg = fwk_mmio_read_32((uintptr_t)(gicr_reg + GICR_WAKER));
    TEST_ASSERT_EQUAL(0, reg);

    /* Verify GICD_IROUTER<n> */
    irouter = fwk_mmio_read_64(
        (uintptr_t)(gicd_reg + GICD_IROUTER(INTERRUPT_ID_PPI_LIMIT)));
    TEST_ASSERT_EQUAL(irouter, 0x7);
}

int template_test_main(void)
{
    UNITY_BEGIN();
    RUN_TEST(test_mod_gicx00_init);
    RUN_TEST(test_mod_gicx00_init_gic720ae);
    return UNITY_END();
}

#if !defined(TEST_ON_TARGET)
int main(void)
{
    return template_test_main();
}
#endif
