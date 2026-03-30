/*
 * Arm SCP/MCP Software
 * Copyright (c) 2024-2025, Arm Limited and Contributors. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include "unity.h"

#include <ppu_v1.h>

#include <mod_timer.h>

#include <stdbool.h>
#include <stdint.h>
#include <string.h>

#include UNIT_TEST_SRC
#define WRITE_PATTERN 0x11223344

#define PPU_V1_OPMODE_TIMEOUT (100000u)

/*
 * Mirror of the register block.
 */
struct ppu_v1_ppu_reg_mut {
    uint32_t PWPR;
    uint32_t PMER;
    uint32_t PWSR;
    uint32_t RESERVED0;
    uint32_t DISR;
    uint32_t MISR;
    uint32_t STSR;
    uint32_t UNLK;
    uint32_t PWCR;
    uint32_t PTCR;
    uint32_t RESERVED1[2];
    uint32_t IMR;
    uint32_t AIMR;
    uint32_t ISR;
    uint32_t AISR;
    uint32_t IESR;
    uint32_t OPSR;
    uint32_t RESERVED2[2];
    uint32_t FUNRR;
    uint32_t FULRR;
    uint32_t MEMRR;
    uint8_t RESERVED3[0x160 - 0x5C];
    uint32_t EDTR0;
    uint32_t EDTR1;
    uint32_t RESERVED4[2];
    uint32_t DCCR0;
    uint32_t DCCR1;
    uint8_t RESERVED5[0xFB0 - 0x178];
    uint32_t IDR0;
    uint32_t IDR1;
    uint8_t RESERVED6[0xFC8 - 0xFB8];
    uint32_t IIDR;
    uint32_t AIDR;
    uint8_t RESERVED7[0x1000 - 0xFD0];
};

static struct ppu_v1_ppu_reg_mut regs_hw;
static struct ppu_v1_regs regs_wrap;
#ifdef BUILD_HAS_AE_EXTENSION
static struct ppu_v1_cluster_ae_reg cluster_ae_mut;
#endif

static void init_fake_ppu_regs(uint32_t idr0, uint32_t idr1, uint32_t aidr)
{
    memset(&regs_hw, 0, sizeof(regs_hw));
    regs_hw.IDR0 = idr0;
    regs_hw.IDR1 = idr1;
    regs_hw.AIDR = aidr;

    regs_wrap.ppu_reg = (struct ppu_v1_ppu_reg *)&regs_hw;

#ifdef BUILD_HAS_AE_EXTENSION
    regs_wrap.cluster_ae_reg = (struct ppu_v1_cluster_ae_reg *)&cluster_ae_mut;
#endif
}

static int fake_wait(
    fwk_id_t timer_id,
    uint32_t delay_us,
    bool (*condition)(void *data),
    void *data)
{
    (void)timer_id;
    (void)delay_us;

    if (condition(data)) {
        return FWK_SUCCESS;
    }

    return FWK_E_TIMEOUT;
}

static struct mod_timer_api g_fake_timer_api = {
    .wait = fake_wait,
};

static struct ppu_v1_timer_ctx make_tctx(void)
{
    struct ppu_v1_timer_ctx t = {
        .timer_id = (fwk_id_t){ 0 },
        .timer_api = &g_fake_timer_api,
        .delay_us = 0,
    };
    return t;
}

void setUp(void)
{
    init_fake_ppu_regs(0x10, 0, 0);

#ifdef PPU_V1_PWSR_PWR_STATUS
#    ifdef PPU_V1_PWSR_PWR_STATUS_POS
    regs_hw.PWSR &= ~PPU_V1_PWSR_PWR_STATUS;
    regs_hw.PWSR |= ((unsigned)PPU_V1_MODE_ON << PPU_V1_PWSR_PWR_STATUS_POS);
#    else
    regs_hw.PWSR |= PPU_V1_PWSR_PWR_STATUS_ON;
#    endif
#else
    regs_hw.PWSR |= PPU_V1_PWSR_PWR_STATUS_ON;
#endif

#ifdef PPU_V1_PWSR_OP_STATUS
    regs_hw.PWSR &= ~PPU_V1_PWSR_OP_STATUS;
    regs_hw.PWSR |= (3u << PPU_V1_PWSR_OP_STATUS_POS);
#else
    regs_hw.PWSR |= (3u << PPU_V1_PWSR_OP_STATUS_POS);
#endif
}

void tearDown(void)
{
}

#ifdef BUILD_HAS_AE_EXTENSION
void test_ppu_v1_write_ppu_reg(void)
{
    struct ppu_v1_regs ppu;
    struct ppu_v1_ppu_reg ppu_reg;
    struct ppu_v1_cluster_ae_reg cluster_ae_reg;

    ppu.ppu_reg = &ppu_reg;
    ppu.cluster_ae_reg = &cluster_ae_reg;
    ppu_reg.PWPR = 0;
    cluster_ae_reg.CLUSTERWRITEKEY = 0;

    ppu_v1_write_ppu_reg(&ppu, &ppu_reg.PWPR, WRITE_PATTERN);
    TEST_ASSERT_EQUAL(ppu_reg.PWPR, WRITE_PATTERN);
    TEST_ASSERT_EQUAL(cluster_ae_reg.CLUSTERWRITEKEY, CLUSTER_AE_KEY_VALUE);
}
#else
void test_ppu_v1_write_ppu_reg(void)
{
    struct ppu_v1_regs ppu;
    struct ppu_v1_ppu_reg ppu_reg;

    ppu.ppu_reg = &ppu_reg;
    ppu_reg.PWPR = 0;

    ppu_v1_write_ppu_reg(&ppu, &ppu_reg.PWPR, WRITE_PATTERN);
    TEST_ASSERT_EQUAL(ppu_reg.PWPR, WRITE_PATTERN);
}
#endif

void test_set_operating_mode_reject_when_not_on(void)
{
    struct ppu_v1_timer_ctx tctx = make_tctx();
    regs_hw.PWSR = 0;

    TEST_ASSERT_NOT_EQUAL(
        0,
        ppu_v1_set_operating_mode(
            &regs_wrap, (enum ppu_v1_opmode)1, &tctx, PPU_V1_OPMODE_TIMEOUT));
    TEST_ASSERT_EQUAL(0, regs_hw.PWPR & PPU_V1_PWPR_OP_POLICY);
}

void test_set_operating_mode_success_when_on(void)
{
    struct ppu_v1_timer_ctx tctx = make_tctx();
    regs_hw.PWSR = (PPU_V1_MODE_ON << PPU_V1_PWSR_PWR_STATUS_POS) |
        (2u << PPU_V1_PWSR_OP_STATUS_POS);

    TEST_ASSERT_EQUAL(
        0,
        ppu_v1_set_operating_mode(
            &regs_wrap, (enum ppu_v1_opmode)2, &tctx, PPU_V1_OPMODE_TIMEOUT));
    TEST_ASSERT_EQUAL(
        (2u << PPU_V1_PWPR_OP_POLICY_POS),
        regs_hw.PWPR & PPU_V1_PWPR_OP_POLICY);
}

void test_enable_dynamic_opmode_success(void)
{
    struct ppu_v1_timer_ctx tctx = make_tctx();

    regs_hw.PWSR |= PPU_V1_PWSR_OP_DYN_STATUS;

    TEST_ASSERT_EQUAL(
        FWK_SUCCESS,
        ppu_v1_opmode_dynamic_enable(
            &regs_wrap,
            /*enable=*/true,
            (enum ppu_v1_opmode)1,
            &tctx,
            PPU_V1_OPMODE_TIMEOUT));

    TEST_ASSERT_NOT_EQUAL(0, regs_hw.PWPR & PPU_V1_PWPR_OP_DYN_EN);
}

void test_get_operating_mode(void)
{
    printf("DEBUG PWSR=0x%08x\n", regs_hw.PWSR);
    enum ppu_v1_opmode opm = ppu_v1_get_operating_mode(&regs_wrap);
    TEST_ASSERT_EQUAL((enum ppu_v1_opmode)3, opm);
}

void test_opmode_dynamic_enable_with_timer_success(void)
{
    struct ppu_v1_timer_ctx tctx = make_tctx();

    /* Make fake_wait() see the condition as already satisfied: OP_DYN_STATUS=1
     */
    regs_hw.PWSR |= PPU_V1_PWSR_OP_DYN_STATUS;

    int status = ppu_v1_opmode_dynamic_enable(
        &regs_wrap, true, PPU_V1_OPMODE_00, &tctx, PPU_V1_OPMODE_TIMEOUT);

    TEST_ASSERT_EQUAL(FWK_SUCCESS, status);
    TEST_ASSERT_NOT_EQUAL(0u, regs_hw.PWPR & PPU_V1_PWPR_OP_DYN_EN);
}

void test_opmode_dynamic_disable_with_timer_success(void)
{
    struct ppu_v1_timer_ctx tctx = make_tctx();

    /* Make fake_wait() see the condition as already satisfied: OP_DYN_STATUS=0
     */
    regs_hw.PWSR &= ~PPU_V1_PWSR_OP_DYN_STATUS;

    int status = ppu_v1_opmode_dynamic_enable(
        &regs_wrap, false, 0, &tctx, PPU_V1_OPMODE_TIMEOUT);

    TEST_ASSERT_EQUAL(FWK_SUCCESS, status);
    TEST_ASSERT_EQUAL(0u, regs_hw.PWPR & PPU_V1_PWPR_OP_DYN_EN);
}

void test_opmode_dynamic_enable_no_timer_ctx_returns_param(void)
{
    /* In a timer-enabled build, NULL timer_ctx must return FWK_E_PARAM */
    int status = ppu_v1_opmode_dynamic_enable(
        &regs_wrap, true, PPU_V1_OPMODE_00, NULL, PPU_V1_OPMODE_TIMEOUT);

    TEST_ASSERT_EQUAL(FWK_E_TIMEOUT, status);
}

#ifndef BUILD_HAS_MOD_TIMER
void test_opmode_dynamic_disable_spin_times_out(void)
{
    /* No timer support: bounded spin path; keep OP_DYN_STATUS stuck at 1 */
    regs_hw.PWSR |= PPU_V1_PWSR_OP_DYN_STATUS;

    int status = ppu_v1_opmode_dynamic_enable(
        &regs_wrap, false, 0, NULL, 10); /* small bounded spin */

    TEST_ASSERT_EQUAL(FWK_E_TIMEOUT, status);
}
#endif /* !BUILD_HAS_MOD_TIMER */

void test_opmode_dynamic_enable_invalid_mode_returns_param(void)
{
    struct ppu_v1_timer_ctx tctx = make_tctx();

    int status = ppu_v1_opmode_dynamic_enable(
        &regs_wrap, true, PPU_V1_OPMODE_COUNT, &tctx, PPU_V1_OPMODE_TIMEOUT);

    TEST_ASSERT_EQUAL(FWK_E_PARAM, status);
}

int ppu_v1_test_main(void)
{
    UNITY_BEGIN();

    RUN_TEST(test_ppu_v1_write_ppu_reg);
    RUN_TEST(test_set_operating_mode_reject_when_not_on);
    RUN_TEST(test_set_operating_mode_success_when_on);
    RUN_TEST(test_enable_dynamic_opmode_success);
    RUN_TEST(test_get_operating_mode);
    RUN_TEST(test_opmode_dynamic_enable_with_timer_success);
    RUN_TEST(test_opmode_dynamic_disable_with_timer_success);
    RUN_TEST(test_opmode_dynamic_enable_no_timer_ctx_returns_param);
    RUN_TEST(test_opmode_dynamic_disable_spin_times_out);
    RUN_TEST(test_opmode_dynamic_enable_invalid_mode_returns_param);

    return UNITY_END();
}

int main(void)
{
    return ppu_v1_test_main();
}
