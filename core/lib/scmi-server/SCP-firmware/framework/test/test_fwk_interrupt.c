/*
 * Arm SCP/MCP Software
 * Copyright (c) 2015-2025, Arm Limited and Contributors. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */
#include "interrupt_functions.h"

#include <internal/fwk_interrupt.h>

#include <fwk_arch.h>
#include <fwk_assert.h>
#include <fwk_interrupt.h>
#include <fwk_macros.h>
#include <fwk_status.h>
#include <fwk_test.h>

#include <stdbool.h>
#include <stddef.h>

#define INTERRUPT_ID 42

/*
 * Variables for the mock functions
 */

void test_case_setup(void)
{
    init_return_val = FWK_E_HANDLER;
    is_enabled_return_val = FWK_E_HANDLER;
    enable_return_val = FWK_E_HANDLER;
    disable_return_val = FWK_E_HANDLER;
    is_pending_return_val = FWK_E_HANDLER;
    set_pending_return_val = FWK_E_HANDLER;
    clear_pending_return_val = FWK_E_HANDLER;
    set_isr_return_val = FWK_E_HANDLER;
    set_isr_param_return_val = FWK_E_HANDLER;
    set_isr_nmi_return_val = FWK_E_HANDLER;
    set_isr_nmi_param_return_val = FWK_E_HANDLER;
    set_isr_fault_return_val = FWK_E_HANDLER;
    get_current_return_val = FWK_E_HANDLER;
    configure_return_val = FWK_E_HANDLER;
    set_intr_priority_return_val = FWK_E_HANDLER;
}

static void test_fwk_interrupt_init(void)
{
    int result;

    init_return_val = FWK_SUCCESS;
    result = fwk_arch_interrupt_init();
    assert(result == FWK_SUCCESS);
}

static void test_fwk_interrupt_critical_section(void)
{
    unsigned int flags;

    flags = fwk_interrupt_global_disable();
    assert(critical_section_nest_level == 1);

    fwk_interrupt_global_enable(flags);
    assert(critical_section_nest_level == 0);
}

static void test_fwk_interrupt_is_enabled(void)
{
    bool state;
    int result;

    result = fwk_interrupt_is_enabled(INTERRUPT_ID, NULL);
    assert(result == FWK_E_PARAM);

    is_enabled_return_val = FWK_SUCCESS;
    result = fwk_interrupt_is_enabled(INTERRUPT_ID, &state);
    assert(result == FWK_SUCCESS);
}

static void test_fwk_interrupt_enable(void)
{
    int result;

    enable_return_val = FWK_SUCCESS;
    result = fwk_interrupt_enable(INTERRUPT_ID);
    assert(result == FWK_SUCCESS);
}

static void test_fwk_interrupt_disable(void)
{
    int result;

    disable_return_val = FWK_SUCCESS;
    result = fwk_interrupt_disable(INTERRUPT_ID);
    assert(result == FWK_SUCCESS);
}

static void test_fwk_interrupt_is_pending(void)
{
    int result;
    bool state;

    result = fwk_interrupt_is_pending(INTERRUPT_ID, NULL);
    assert(result == FWK_E_PARAM);

    is_pending_return_val = FWK_SUCCESS;
    result = fwk_interrupt_is_pending(INTERRUPT_ID, &state);
    assert(result == FWK_SUCCESS);
}

static void test_fwk_interrupt_set_pending(void)
{
    int result;

    set_pending_return_val = FWK_SUCCESS;
    result = fwk_interrupt_set_pending(INTERRUPT_ID);
    assert(result == FWK_SUCCESS);
}

static void test_fwk_interrupt_clear_pending(void)
{
    int result;

    clear_pending_return_val = FWK_SUCCESS;
    result = fwk_interrupt_clear_pending(INTERRUPT_ID);
    assert(result == FWK_SUCCESS);
}

static void test_fwk_interrupt_set_isr(void)
{
    int result;

    result = fwk_interrupt_set_isr(INTERRUPT_ID, NULL);
    assert(result == FWK_E_PARAM);

    set_isr_return_val = FWK_SUCCESS;
    result = fwk_interrupt_set_isr(INTERRUPT_ID, fake_isr);
    assert(result == FWK_SUCCESS);

    set_isr_return_val = FWK_E_HANDLER;
    set_isr_nmi_return_val = FWK_SUCCESS;
    result = fwk_interrupt_set_isr(FWK_INTERRUPT_NMI, fake_isr);
    assert(result == FWK_SUCCESS);
}

static void test_fwk_interrupt_set_isr_param(void)
{
    int result;

    result = fwk_interrupt_set_isr_param(INTERRUPT_ID, NULL, 0);
    assert(result == FWK_E_PARAM);

    set_isr_param_return_val = FWK_SUCCESS;
    result = fwk_interrupt_set_isr_param(INTERRUPT_ID, fake_isr_param, 0);
    assert(result == FWK_SUCCESS);

    set_isr_param_return_val = FWK_E_HANDLER;
    set_isr_nmi_param_return_val = FWK_SUCCESS;
    result = fwk_interrupt_set_isr_param(FWK_INTERRUPT_NMI, fake_isr_param, 0);
    assert(result == FWK_SUCCESS);
}

static void test_fwk_interrupt_set_isr_fault(void)
{
    int result;

    result = fwk_interrupt_set_isr_fault(NULL);
    assert(result == FWK_E_PARAM);

    set_isr_fault_return_val = FWK_SUCCESS;
    result = fwk_interrupt_set_isr_fault(fake_isr);
    assert(result == FWK_SUCCESS);
}

static void test_fwk_interrupt_get_current(void)
{
    int result;
    unsigned int interrupt;

    result = fwk_interrupt_get_current(NULL);
    assert(result == FWK_E_PARAM);

    get_current_return_val = FWK_SUCCESS;
    result = fwk_interrupt_get_current(&interrupt);
    assert(result == FWK_SUCCESS);
}

static void test_fwk_interrupt_nested_critical_section(void)
{
    unsigned int flags1, flags2, flags3;
    flags1 = fwk_interrupt_global_disable();
    assert(critical_section_nest_level == 1);

    flags2 = fwk_interrupt_global_disable();
    assert(critical_section_nest_level == 2);

    flags3 = fwk_interrupt_global_disable();
    assert(critical_section_nest_level == 3);

    fwk_interrupt_global_enable(flags3);
    assert(critical_section_nest_level == 2);

    fwk_interrupt_global_enable(flags2);
    assert(critical_section_nest_level == 1);

    fwk_interrupt_global_enable(flags1);
    assert(critical_section_nest_level == 0);
}

static void test_fwk_interrupt_configure(void)
{
    int result;

    configure_return_val = FWK_SUCCESS;
    result = fwk_interrupt_configure(INTERRUPT_ID, 0);
    assert(result == FWK_SUCCESS);
}

static void test_fwk_interrupt_set_priority(void)
{
    int result;

    set_intr_priority_return_val = FWK_SUCCESS;
    result = fwk_interrupt_set_priority(INTERRUPT_ID, 0);
    assert(result == FWK_SUCCESS);
}

static const struct fwk_test_case_desc test_case_table[] = {
    FWK_TEST_CASE(test_fwk_interrupt_init),
    FWK_TEST_CASE(test_fwk_interrupt_critical_section),
    FWK_TEST_CASE(test_fwk_interrupt_is_enabled),
    FWK_TEST_CASE(test_fwk_interrupt_enable),
    FWK_TEST_CASE(test_fwk_interrupt_disable),
    FWK_TEST_CASE(test_fwk_interrupt_is_pending),
    FWK_TEST_CASE(test_fwk_interrupt_set_pending),
    FWK_TEST_CASE(test_fwk_interrupt_clear_pending),
    FWK_TEST_CASE(test_fwk_interrupt_set_isr),
    FWK_TEST_CASE(test_fwk_interrupt_set_isr_param),
    FWK_TEST_CASE(test_fwk_interrupt_set_isr_fault),
    FWK_TEST_CASE(test_fwk_interrupt_get_current),
    FWK_TEST_CASE(test_fwk_interrupt_nested_critical_section),
    FWK_TEST_CASE(test_fwk_interrupt_configure),
    FWK_TEST_CASE(test_fwk_interrupt_set_priority),
};

struct fwk_test_suite_desc test_suite = {
    .name = "fwk_interrupt",
    .test_case_setup = test_case_setup,
    .test_case_count = FWK_ARRAY_SIZE(test_case_table),
    .test_case_table = test_case_table,
};
