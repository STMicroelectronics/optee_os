/*
 * Arm SCP/MCP Software
 * Copyright (c) 2022-2025, Arm Limited and Contributors. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include "config_ppu_v1.h"
#include "scp_unity.h"
#include "unity.h"

#include <Mockfwk_id.h>
#include <Mockfwk_mm.h>
#include <Mockfwk_module.h>
#include <Mockfwk_notification.h>
#include <Mockmod_ppu_v1_extra.h>
#include <internal/Mockfwk_core_internal.h>

#include <mod_ppu_v1_extra.h>

#include <fwk_element.h>
#include <fwk_macros.h>
#include <fwk_notification.h>

#include <string.h>

#include UNIT_TEST_SRC

#define CORES_PER_CLUSTER 2

static struct ppu_v1_pd_ctx pd_table[PD_COUNT];

static struct mod_timer_alarm_api alarm_api_driver = {
    .start = start_alarm_api,
    .stop = stop_alarm_api,
};

static int ut_ppu_notify(fwk_id_t elem_id, unsigned state)
{
    struct fwk_event ev = { 0 };
    struct fwk_event resp = { 0 };

    ev.source_id = FWK_ID_NONE;
    ev.target_id = elem_id;
    ev.id = FWK_ID_NONE;

    struct mod_pd_power_state_transition_notification_params params = {
        .state = state,
    };
    memcpy(ev.params, &params, sizeof(params));

    /* process_notification() checks the id matches; keep your stubs */
    fwk_id_is_type_ExpectAnyArgsAndReturn(true);
    fwk_id_build_module_id_ExpectAnyArgsAndReturn(FWK_ID_NONE);
    fwk_module_get_data_ExpectAnyArgsAndReturn(&ppu_v1_config_data_ut);
    fwk_id_is_equal_ExpectAnyArgsAndReturn(true);

    /* Only needed if state == ON (since OFF returns early) */
    if (state == MOD_PD_STATE_ON) {
        fwk_id_get_element_idx_ExpectAnyArgsAndReturn(PD_PPU_IDX_3);
    }

    return ppu_v1_process_notification(&ev, &resp);
}

static int stub_report_transition(fwk_id_t id, unsigned state)
{
    (void)id;
    (void)state;
    return FWK_SUCCESS;
}

static struct mod_pd_driver_input_api g_stub_pd_in = {
    .report_power_state_transition = stub_report_transition,
};

void setUp(void)
{
    memset(&ppu_v1_ctx, 0, sizeof(ppu_v1_ctx));
    ppu_v1_ctx.pd_ctx_table_size = PD_COUNT;
    ppu_v1_ctx.pd_ctx_table = pd_table;

    for (unsigned i = 0; i < PD_COUNT; ++i) {
        ppu_v1_ctx.pd_ctx_table[i].config = &pd_ppu_ctx_config[i];
        ppu_v1_ctx.pd_ctx_table[i].alarm_api = &alarm_api_driver;
        ppu_v1_ctx.pd_ctx_table[i].pd_driver_input_api = &g_stub_pd_in;

        ppu_v1_ctx.pd_ctx_table[i].opmode_timeout =
            ppu_v1_ctx.pd_ctx_table[i].config->opmode_time_out;
    }

    Mockfwk_id_Init();
    Mockfwk_mm_Init();
    Mockfwk_module_Init();
    Mockfwk_notification_Init();
    Mockmod_ppu_v1_extra_Init();
    Mockfwk_core_internal_Init();
}

void tearDown(void)
{
    Mockfwk_core_internal_Verify();
    Mockmod_ppu_v1_extra_Verify();
    Mockfwk_notification_Verify();
    Mockfwk_module_Verify();
    Mockfwk_mm_Verify();
    Mockfwk_id_Verify();

    Mockfwk_core_internal_Destroy();
    Mockmod_ppu_v1_extra_Destroy();
    Mockfwk_notification_Destroy();
    Mockfwk_module_Destroy();
    Mockfwk_mm_Destroy();
    Mockfwk_id_Destroy();
}

void test_ppu_v1_pd_init_error(void)
{
    int status;
    fwk_id_t pd_id;
    unsigned int unused = 0;
    struct mod_ppu_v1_pd_config config;

    config.pd_type = MOD_PD_TYPE_COUNT + 1;
    status = ppu_v1_pd_init(pd_id, unused, &config);
    TEST_ASSERT_EQUAL(status, FWK_E_DATA);
}

void test_ppu_v1_pd_init(void)
{
    int status;
    fwk_id_t pd_id;
    unsigned int unused = 0;
    struct mod_ppu_v1_pd_config config;

    config.pd_type = MOD_PD_TYPE_CLUSTER;
    config.timer_config = NULL;
    config.ppu.irq = FWK_INTERRUPT_NONE;
    config.default_power_on = false;

    fwk_id_get_element_idx_ExpectAnyArgsAndReturn(0);
    fwk_optional_id_is_defined_ExpectAnyArgsAndReturn(false);

    struct ppu_v1_pd_ctx *core_pd_ctx_table_temp[CORES_PER_CLUSTER];
    static struct ppu_v1_pd_ctx p0;
    static struct ppu_v1_pd_ctx p1;

    core_pd_ctx_table_temp[0] = &p0;
    core_pd_ctx_table_temp[1] = &p1;

    /* Make a local one to get the size for the next malloc */
    struct ppu_v1_cluster_pd_ctx cluster_pd_ctx_temp;

    cluster_pd_ctx_temp.core_pd_ctx_table =
        (struct ppu_v1_pd_ctx **)&core_pd_ctx_table_temp;
    cluster_pd_ctx_temp.core_count = CORES_PER_CLUSTER;

    fwk_mm_calloc_ExpectAndReturn(
        1, sizeof(cluster_pd_ctx_temp), &ppu_v1_ctx.pd_ctx_table);

    fwk_mm_calloc_ExpectAndReturn(
        ppu_v1_ctx.max_num_cores_per_cluster,
        sizeof(core_pd_ctx_table_temp[0]),
        &core_pd_ctx_table_temp);

    status = ppu_v1_pd_init(pd_id, unused, &config);
    TEST_ASSERT_EQUAL(status, FWK_SUCCESS);
}

void test_ppu_v1_mod_init(void)
{
    fwk_id_t mod_id;
    int status;

    fwk_mm_calloc_ExpectAndReturn(
        PD_COUNT, sizeof(struct ppu_v1_pd_ctx), &pd_table);

    /* Clear to ensure it gets reset */
    ppu_v1_ctx.pd_ctx_table_size = 0;
    ppu_v1_ctx.max_num_cores_per_cluster = 0;

    status = ppu_v1_mod_init(mod_id, PD_COUNT, &ppu_v1_config_data_ut);
    TEST_ASSERT_EQUAL(status, FWK_SUCCESS);
    TEST_ASSERT_EQUAL(ppu_v1_ctx.pd_ctx_table_size, PD_COUNT);
    TEST_ASSERT_EQUAL(ppu_v1_ctx.max_num_cores_per_cluster, CORES_PER_CLUSTER);
}

void test_ppu_v1_core_pd_set_state_sleep(void)
{
    int status;
    fwk_id_t core_pd_id;
    struct ppu_v1_pd_ctx *pd_ctx_temp;

    pd_ctx_temp = &ppu_v1_ctx.pd_ctx_table[0];

    fwk_id_get_element_idx_ExpectAnyArgsAndReturn(0);
    ppu_v1_dynamic_enable_ExpectAnyArgs();

    ppu_v1_lock_off_enable_ExpectAnyArgs();
    ppu_v1_interrupt_unmask_ExpectAnyArgs();

    ppu_v1_set_input_edge_sensitivity_Expect(
        &pd_ctx_temp->ppu, PPU_V1_MODE_ON, PPU_V1_EDGE_SENSITIVITY_MASKED);

    fwk_id_get_element_idx_ExpectAnyArgsAndReturn(0);
    fwk_id_get_element_idx_ExpectAnyArgsAndReturn(0);
    start_alarm_api_ExpectAndReturn(
        ppu_v1_ctx.pd_ctx_table[0].config->alarm_id,
        ppu_v1_ctx.pd_ctx_table[0].config->alarm_delay,
        MOD_TIMER_ALARM_TYPE_ONCE,
        deeper_locking_alarm_callback,
        0,
        FWK_SUCCESS);

    status = ppu_v1_core_pd_set_state(core_pd_id, MOD_PD_STATE_SLEEP);
    TEST_ASSERT_EQUAL(FWK_SUCCESS, status);
}

void test_start_deeper_locking_alarm(void)
{
    int status;
    fwk_id_t core_pd_id;

    fwk_id_get_element_idx_ExpectAnyArgsAndReturn(0);
    fwk_id_get_element_idx_ExpectAnyArgsAndReturn(0);

    start_alarm_api_ExpectAndReturn(
        ppu_v1_ctx.pd_ctx_table[0].config->alarm_id,
        ppu_v1_ctx.pd_ctx_table[0].config->alarm_delay,
        MOD_TIMER_ALARM_TYPE_ONCE,
        deeper_locking_alarm_callback,
        0,
        FWK_SUCCESS);

    status = start_deeper_locking_alarm(core_pd_id);

    TEST_ASSERT_EQUAL(status, FWK_SUCCESS);
}

void test_start_deeper_locking_alarm_null_api(void)
{
    int status;
    fwk_id_t core_pd_id;

    ppu_v1_ctx.pd_ctx_table[0].alarm_api = NULL;

    fwk_id_get_element_idx_ExpectAnyArgsAndReturn(0);

    status = start_deeper_locking_alarm(core_pd_id);

    TEST_ASSERT_EQUAL(status, FWK_E_SUPPORT);
}

void test_deeper_locking_alarm_callback(void)
{
    uintptr_t param = (uintptr_t)0;
    struct ppu_v1_pd_ctx *pd_ctx_temp;
    pd_ctx_temp = &ppu_v1_ctx.pd_ctx_table[0];

    ppu_v1_lock_off_disable_Expect(&pd_ctx_temp->ppu);
    ppu_v1_off_unlock_ExpectAnyArgs();

    deeper_locking_alarm_callback(param);
}

void test_pd_init_no_opmode_when_disabled(void)
{
    int status;
    fwk_id_t pd_id;
    unsigned int unused = 0;
    struct mod_ppu_v1_pd_config config = { 0 };
    config.pd_type = MOD_PD_TYPE_DEVICE;
    config.enable_opmode_support = false; /* ensure disabled */
    config.ppu.irq = FWK_INTERRUPT_NONE;

    fwk_id_get_element_idx_ExpectAnyArgsAndReturn(0);
    fwk_optional_id_is_defined_ExpectAnyArgsAndReturn(false);

    status = ppu_v1_pd_init(pd_id, unused, &config);
    TEST_ASSERT_EQUAL(FWK_SUCCESS, status);
}

void test_pd_init_with_opmode_enabled_basic(void)
{
    int status;
    fwk_id_t pd_id;
    unsigned int unused = 0;
    struct mod_ppu_v1_pd_config config = { 0 };
    config.pd_type = MOD_PD_TYPE_DEVICE;
    config.enable_opmode_support = true; /* request enable */
    config.enable_opmode_dynamic_policy = true;
    config.min_op_mode = 2;
    config.ppu.irq = FWK_INTERRUPT_NONE;

    fwk_id_get_element_idx_ExpectAnyArgsAndReturn(0);
    fwk_optional_id_is_defined_ExpectAnyArgsAndReturn(false);

    status = ppu_v1_pd_init(pd_id, unused, &config);
    TEST_ASSERT_EQUAL(FWK_SUCCESS, status);
}

void test_systop_init_on_enables_opmode_dynamic_and_unmasks_irqs(void)
{
    int status;
    fwk_id_t sys_id = FWK_ID_ELEMENT_INIT(FWK_MODULE_IDX_PPU_V1, PD_PPU_IDX_3);
    struct ppu_v1_pd_ctx *ctx = &ppu_v1_ctx.pd_ctx_table[PD_PPU_IDX_3];
    struct ppu_v1_regs *ppu = &ctx->ppu;

    ctx->opmode_enabled = true;
    ctx->opmode_dyn_policy_enabled = true;
    ctx->opmode_irqs_enabled = true;

    ppu_v1_init_Expect(ppu);
    ppu_v1_get_num_opmode_ExpectAndReturn(ppu, 4);

    ppu_v1_opmode_dynamic_enable_ExpectAndReturn(
        ppu,
        true,
        PPU_V1_OPMODE_00,
        ctx->timer_ctx,
        ctx->opmode_timeout,
        FWK_SUCCESS);
    ppu_v1_additional_interrupt_unmask_Expect(
        ppu, PPU_V1_AIMR_STA_POLICY_OP_IRQ_MASK);
    ppu_v1_additional_interrupt_unmask_Expect(
        ppu, PPU_V1_AIMR_UNSPT_POLICY_IRQ_MASK);

    status = ut_ppu_notify(sys_id, MOD_PD_STATE_ON);
    TEST_ASSERT_EQUAL(FWK_SUCCESS, status);
}

void test_systop_init_off_does_not_touch_opmode(void)
{
    int status;
    fwk_id_t sys_id = FWK_ID_ELEMENT_INIT(FWK_MODULE_IDX_PPU_V1, PD_PPU_IDX_3);

    ppu_v1_opmode_dynamic_enable_StopIgnore();
    ppu_v1_additional_interrupt_unmask_StopIgnore();
    ppu_v1_request_operating_mode_StopIgnore();

    status = ut_ppu_notify(sys_id, MOD_PD_STATE_OFF);
    TEST_ASSERT_EQUAL(FWK_SUCCESS, status);
}

void test_systop_on_applies_opmode_via_request_async(void)
{
    int status;
    fwk_id_t sys_id = FWK_ID_ELEMENT_INIT(FWK_MODULE_IDX_PPU_V1, PD_PPU_IDX_3);
    struct ppu_v1_pd_ctx *ctx = &ppu_v1_ctx.pd_ctx_table[PD_PPU_IDX_3];
    struct ppu_v1_regs *ppu = &ctx->ppu;

    ctx->opmode_enabled = true;
    ctx->opmode_irqs_enabled = true;
    ctx->opmode_dyn_policy_enabled = false;

    ppu_v1_is_dynamic_enabled_ExpectAndReturn(ppu, false);
    fwk_id_get_element_idx_ExpectAnyArgsAndReturn(PD_PPU_IDX_3);
    ppu_v1_set_power_mode_ExpectAndReturn(
        ppu, PPU_V1_MODE_ON, ctx->timer_ctx, FWK_SUCCESS);

    ppu_v1_get_power_mode_ExpectAndReturn(ppu, PPU_V1_MODE_ON);
    ppu_v1_get_num_opmode_ExpectAndReturn(ppu, 4);
    ppu_v1_request_operating_mode_ExpectAndReturn(
        ppu, PPU_V1_OPMODE_00, FWK_SUCCESS);
    ppu_v1_get_operating_mode_IgnoreAndReturn((enum ppu_v1_opmode)1);

    status = ppu_v1_pd_set_state(sys_id, MOD_PD_STATE_ON);
    TEST_ASSERT_EQUAL(FWK_SUCCESS, status);
}

void test_systop_isr_policy_complete_clears_pending(void)
{
    struct ppu_v1_pd_ctx *ctx = &ppu_v1_ctx.pd_ctx_table[PD_PPU_IDX_3];
    struct ppu_v1_regs *ppu = &ctx->ppu;

    ctx->opmode_pending = true;
    ctx->opmode_target = PPU_V1_OPMODE_00;

    ppu_v1_is_additional_interrupt_pending_ExpectAndReturn(
        ppu, PPU_V1_AISR_STA_POLICY_OP_IRQ, true);
    ppu_v1_ack_additional_interrupt_Expect(ppu, PPU_V1_AISR_STA_POLICY_OP_IRQ);

    ppu_v1_is_additional_interrupt_pending_ExpectAndReturn(
        ppu, PPU_V1_AISR_UNSPT_POLICY_IRQ, false);

    ppu_v1_get_operating_mode_ExpectAndReturn(&ctx->ppu, PPU_V1_OPMODE_00);

    ppu_interrupt_handler((uintptr_t)ctx);

    TEST_ASSERT_FALSE(ctx->opmode_pending);
}

void test_systop_on_sync_path_sets_opmode_when_irqs_disabled(void)
{
    int status;
    fwk_id_t sys_id = FWK_ID_ELEMENT_INIT(FWK_MODULE_IDX_PPU_V1, PD_PPU_IDX_3);
    struct ppu_v1_pd_ctx *ctx = &ppu_v1_ctx.pd_ctx_table[PD_PPU_IDX_3];
    struct ppu_v1_regs *ppu = &ctx->ppu;

    ctx->opmode_enabled = true;
    ctx->opmode_irqs_enabled = false;
    ctx->opmode_dyn_policy_enabled = false;

    struct mod_ppu_v1_pd_config mutable_cfg = *ctx->config;
    mutable_cfg.use_opmode_irqs = false;
    ctx->config = &mutable_cfg;

    ppu_v1_is_dynamic_enabled_ExpectAndReturn(ppu, false);
    fwk_id_get_element_idx_ExpectAnyArgsAndReturn(PD_PPU_IDX_3);
    ppu_v1_set_power_mode_ExpectAndReturn(
        ppu, PPU_V1_MODE_ON, ctx->timer_ctx, FWK_SUCCESS);

    ppu_v1_get_power_mode_ExpectAndReturn(ppu, PPU_V1_MODE_ON);
    ppu_v1_get_num_opmode_ExpectAndReturn(ppu, 4);
    ppu_v1_set_operating_mode_ExpectAndReturn(
        ppu,
        PPU_V1_OPMODE_00,
        ctx->timer_ctx,
        ctx->opmode_timeout,
        FWK_SUCCESS);
    ppu_v1_get_operating_mode_IgnoreAndReturn((enum ppu_v1_opmode)1);

    status = ppu_v1_pd_set_state(sys_id, MOD_PD_STATE_ON);
    TEST_ASSERT_EQUAL(FWK_SUCCESS, status);
}

void test_opmode_ctrl_set_enabled_false_clears_pending(void)
{
    fwk_id_t sys_id = FWK_ID_ELEMENT_INIT(FWK_MODULE_IDX_PPU_V1, PD_PPU_IDX_3);
    struct ppu_v1_pd_ctx *ctx = &ppu_v1_ctx.pd_ctx_table[PD_PPU_IDX_3];

    /* Mimic mid transition */
    ctx->opmode_pending = true;

    fwk_id_get_element_idx_IgnoreAndReturn(PD_PPU_IDX_3);

    int status = opmode_set_enabled(sys_id, false);

    TEST_ASSERT_EQUAL(FWK_SUCCESS, status);
    TEST_ASSERT_FALSE(ctx->opmode_pending);
}

void test_opmode_ctrl_set_enabled_true_applies_default_when_on_async(void)
{
    fwk_id_t sys_id = FWK_ID_ELEMENT_INIT(FWK_MODULE_IDX_PPU_V1, PD_PPU_IDX_3);
    struct ppu_v1_pd_ctx *ctx = &ppu_v1_ctx.pd_ctx_table[PD_PPU_IDX_3];
    struct ppu_v1_regs *ppu = &ctx->ppu;

    fwk_id_get_element_idx_IgnoreAndReturn(PD_PPU_IDX_3);

    ctx->opmode_irqs_enabled = true;
    ctx->min_opmode = PPU_V1_OPMODE_00;
    ctx->opmode_dyn_policy_enabled = false;

    ppu_v1_is_dynamic_enabled_IgnoreAndReturn(false);
    ppu_v1_get_power_mode_IgnoreAndReturn(PPU_V1_MODE_ON);

    /* tolerate multiple internal queries */
    ppu_v1_get_num_opmode_IgnoreAndReturn(4);
    ppu_v1_get_operating_mode_IgnoreAndReturn(PPU_V1_OPMODE_01);
    ppu_v1_request_operating_mode_ExpectAndReturn(
        ppu, PPU_V1_OPMODE_00, FWK_SUCCESS);

    int status = opmode_set_enabled(sys_id, true);

    TEST_ASSERT_EQUAL(FWK_SUCCESS, status);
    TEST_ASSERT_TRUE(ctx->opmode_enabled);
    TEST_ASSERT_TRUE(ctx->opmode_pending);
    TEST_ASSERT_EQUAL(PPU_V1_OPMODE_00, ctx->opmode_target);
}

void test_opmode_ctrl_set_dynamic_policy_requires_on(void)
{
    fwk_id_t sys_id = FWK_ID_ELEMENT_INIT(FWK_MODULE_IDX_PPU_V1, PD_PPU_IDX_3);
    struct ppu_v1_pd_ctx *ctx = &ppu_v1_ctx.pd_ctx_table[PD_PPU_IDX_3];

    /* enabled, but PD not ON */
    ctx->opmode_enabled = true;

    fwk_id_get_element_idx_IgnoreAndReturn(PD_PPU_IDX_3);
    ppu_v1_get_power_mode_ExpectAndReturn(&ctx->ppu, PPU_V1_MODE_OFF);

    int status = opmode_enable_dynamic_policy(sys_id, true, PPU_V1_OPMODE_00);
    TEST_ASSERT_EQUAL(FWK_SUCCESS, status);
}

void test_opmode_ctrl_set_dynamic_policy_enables_hw_when_on(void)
{
    fwk_id_t sys_id = FWK_ID_ELEMENT_INIT(FWK_MODULE_IDX_PPU_V1, PD_PPU_IDX_3);
    struct ppu_v1_pd_ctx *ctx = &ppu_v1_ctx.pd_ctx_table[PD_PPU_IDX_3];

    ctx->opmode_enabled = true;

    fwk_id_get_element_idx_IgnoreAndReturn(PD_PPU_IDX_3);
    ppu_v1_get_power_mode_ExpectAndReturn(&ctx->ppu, PPU_V1_MODE_ON);

    ppu_v1_opmode_dynamic_enable_ExpectAndReturn(
        &ctx->ppu,
        true,
        PPU_V1_OPMODE_00,
        ctx->timer_ctx,
        ctx->opmode_timeout,
        FWK_SUCCESS);

    int status = opmode_enable_dynamic_policy(sys_id, true, PPU_V1_OPMODE_00);
    TEST_ASSERT_EQUAL(FWK_SUCCESS, status);
    TEST_ASSERT_TRUE(ctx->opmode_dyn_policy_enabled);
}

void test_opmode_ctrl_set_default_requests_when_on_and_irqs_enabled(void)
{
    fwk_id_t sys_id = FWK_ID_ELEMENT_INIT(FWK_MODULE_IDX_PPU_V1, PD_PPU_IDX_3);
    struct ppu_v1_pd_ctx *ctx = &ppu_v1_ctx.pd_ctx_table[PD_PPU_IDX_3];
    struct ppu_v1_regs *ppu = &ctx->ppu;

    ctx->opmode_enabled = true;
    ctx->opmode_irqs_enabled = true;

    fwk_id_get_element_idx_IgnoreAndReturn(PD_PPU_IDX_3);

    /* PD ON and current is not the new default */
    ppu_v1_get_power_mode_ExpectAndReturn(ppu, PPU_V1_MODE_ON);
    ppu_v1_request_operating_mode_ExpectAndReturn(
        ppu, PPU_V1_OPMODE_00, FWK_SUCCESS);

    int status = opmode_set_min(sys_id, PPU_V1_OPMODE_00);

    TEST_ASSERT_EQUAL(FWK_SUCCESS, status);
    TEST_ASSERT_TRUE(ctx->opmode_pending);
    TEST_ASSERT_EQUAL(PPU_V1_OPMODE_00, ctx->opmode_target);
}

void test_opmode_ctrl_set_default_sets_immediately_when_irqs_disabled(void)
{
    const fwk_id_t pd_id = FWK_ID_ELEMENT(FWK_MODULE_IDX_PPU_V1, 0);

    struct ppu_v1_pd_ctx *ctx = &ppu_v1_ctx.pd_ctx_table[0];

    const enum ppu_v1_opmode requested_opm = PPU_V1_OPMODE_01;

    ctx->opmode_enabled = true;
    ctx->opmode_irqs_enabled = false;
    if (ctx->opmode_timeout == 0) {
        ctx->opmode_timeout = PPU_V1_DEFAULT_OPMODE_TIMEOUT_US;
    }

    fwk_id_get_element_idx_IgnoreAndReturn(0);

    ppu_v1_get_power_mode_ExpectAndReturn(&ctx->ppu, PPU_V1_MODE_ON);
    ppu_v1_set_operating_mode_ExpectAndReturn(
        &ctx->ppu,
        requested_opm,
        ctx->timer_ctx,
        ctx->opmode_timeout,
        FWK_SUCCESS);

    TEST_ASSERT_EQUAL(FWK_SUCCESS, opmode_set_min(pd_id, requested_opm));
    TEST_ASSERT_FALSE(ctx->opmode_pending);
    TEST_ASSERT_EQUAL(requested_opm, ctx->min_opmode);
}

void test_opmode_ctrl_request_now_fails_when_disabled(void)
{
    fwk_id_t sys_id = FWK_ID_ELEMENT_INIT(FWK_MODULE_IDX_PPU_V1, PD_PPU_IDX_3);

    fwk_id_get_element_idx_IgnoreAndReturn(PD_PPU_IDX_3);
    ppu_v1_get_power_mode_IgnoreAndReturn(PPU_V1_MODE_ON);
    ppu_v1_get_num_opmode_IgnoreAndReturn(4);
    ppu_v1_get_operating_mode_IgnoreAndReturn(PPU_V1_OPMODE_00);
    ppu_v1_request_operating_mode_IgnoreAndReturn(FWK_SUCCESS);
    ppu_v1_set_operating_mode_IgnoreAndReturn(FWK_SUCCESS);

    ppu_v1_ctx.pd_ctx_table[PD_PPU_IDX_3].opmode_enabled = false;

    int status = opmode_request_now(sys_id, PPU_V1_OPMODE_00);
    TEST_ASSERT_EQUAL(FWK_E_SUPPORT, status);
}

void test_opmode_ctrl_request_now_requires_on(void)
{
    fwk_id_t sys_id = FWK_ID_ELEMENT_INIT(FWK_MODULE_IDX_PPU_V1, PD_PPU_IDX_3);
    struct ppu_v1_pd_ctx *ctx = &ppu_v1_ctx.pd_ctx_table[PD_PPU_IDX_3];

    ctx->opmode_enabled = true;

    /* Explicit that dynamic policy is off */
    ctx->opmode_dyn_policy_enabled = false;

    fwk_id_get_element_idx_IgnoreAndReturn(PD_PPU_IDX_3);
    ppu_v1_get_power_mode_IgnoreAndReturn(PPU_V1_MODE_OFF);

    int status = opmode_request_now(sys_id, PPU_V1_OPMODE_00);
    TEST_ASSERT_EQUAL(FWK_E_STATE, status);
}

void test_opmode_ctrl_request_now_async_requests_and_clears_pending(void)
{
    fwk_id_t sys_id = FWK_ID_ELEMENT_INIT(FWK_MODULE_IDX_PPU_V1, PD_PPU_IDX_3);
    struct ppu_v1_pd_ctx *ctx = &ppu_v1_ctx.pd_ctx_table[PD_PPU_IDX_3];
    struct ppu_v1_regs *ppu = &ctx->ppu;

    const enum ppu_v1_opmode requested_opm = PPU_V1_OPMODE_02;

    ctx->opmode_enabled = true;
    ctx->opmode_irqs_enabled = true;
    ctx->opmode_dyn_policy_enabled = false;

    fwk_id_get_element_idx_IgnoreAndReturn(PD_PPU_IDX_3);
    ppu_v1_get_power_mode_IgnoreAndReturn(PPU_V1_MODE_ON);
    ppu_v1_request_operating_mode_ExpectAndReturn(
        ppu, requested_opm, FWK_SUCCESS);

    int status = opmode_request_now(sys_id, requested_opm);
    TEST_ASSERT_EQUAL(FWK_SUCCESS, status);
    TEST_ASSERT_TRUE(ctx->opmode_pending);
    TEST_ASSERT_EQUAL(requested_opm, ctx->opmode_target);

    ppu_v1_get_operating_mode_ExpectAndReturn(&ctx->ppu, requested_opm);
    ppu_v1_pd_opmode_irq_complete(ctx);
    TEST_ASSERT_FALSE(ctx->opmode_pending);
}

void test_opmode_ctrl_request_now_sync_sets_with_timeout(void)
{
    fwk_id_t sys_id = FWK_ID_ELEMENT_INIT(FWK_MODULE_IDX_PPU_V1, PD_PPU_IDX_3);
    struct ppu_v1_pd_ctx *ctx = &ppu_v1_ctx.pd_ctx_table[PD_PPU_IDX_3];
    struct ppu_v1_regs *ppu = &ctx->ppu;

    ctx->opmode_enabled = true;
    ctx->opmode_irqs_enabled = false;
    ctx->opmode_timeout = ctx->config->opmode_time_out;
    ctx->opmode_dyn_policy_enabled = false;

    fwk_id_get_element_idx_IgnoreAndReturn(PD_PPU_IDX_3);
    ppu_v1_get_power_mode_ExpectAndReturn(ppu, PPU_V1_MODE_ON);
    ppu_v1_set_operating_mode_ExpectAndReturn(
        ppu,
        PPU_V1_OPMODE_03,
        ctx->timer_ctx,
        ctx->opmode_timeout,
        FWK_SUCCESS);

    int status = opmode_request_now(sys_id, PPU_V1_OPMODE_03);
    TEST_ASSERT_EQUAL(FWK_SUCCESS, status);
}

int mod_ppu_v1_test_main(void)
{
    UNITY_BEGIN();

    RUN_TEST(test_ppu_v1_mod_init);
    RUN_TEST(test_ppu_v1_pd_init_error);
    RUN_TEST(test_ppu_v1_pd_init);
    RUN_TEST(test_ppu_v1_core_pd_set_state_sleep);
    RUN_TEST(test_start_deeper_locking_alarm);
    RUN_TEST(test_start_deeper_locking_alarm_null_api);
    RUN_TEST(test_deeper_locking_alarm_callback);
    RUN_TEST(test_pd_init_no_opmode_when_disabled);
    RUN_TEST(test_pd_init_with_opmode_enabled_basic);
    RUN_TEST(test_systop_init_on_enables_opmode_dynamic_and_unmasks_irqs);
    RUN_TEST(test_systop_init_off_does_not_touch_opmode);
    RUN_TEST(test_systop_on_applies_opmode_via_request_async);
    RUN_TEST(test_systop_isr_policy_complete_clears_pending);
    RUN_TEST(test_systop_on_sync_path_sets_opmode_when_irqs_disabled);
    RUN_TEST(test_opmode_ctrl_set_enabled_false_clears_pending);
    RUN_TEST(test_opmode_ctrl_set_enabled_true_applies_default_when_on_async);
    RUN_TEST(test_opmode_ctrl_set_dynamic_policy_requires_on);
    RUN_TEST(test_opmode_ctrl_set_dynamic_policy_enables_hw_when_on);
    RUN_TEST(test_opmode_ctrl_set_default_requests_when_on_and_irqs_enabled);
    RUN_TEST(test_opmode_ctrl_set_default_sets_immediately_when_irqs_disabled);
    RUN_TEST(test_opmode_ctrl_request_now_fails_when_disabled);
    RUN_TEST(test_opmode_ctrl_request_now_requires_on);
    RUN_TEST(test_opmode_ctrl_request_now_async_requests_and_clears_pending);
    RUN_TEST(test_opmode_ctrl_request_now_sync_sets_with_timeout);

    return UNITY_END();
}

int main(void)
{
    return mod_ppu_v1_test_main();
}
