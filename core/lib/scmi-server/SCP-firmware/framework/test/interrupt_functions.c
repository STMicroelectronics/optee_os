/*
 * Arm SCP/MCP Software
 * Copyright (c) 2025, Arm Limited and Contributors. All rights reserved.
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

int init_return_val;
int is_enabled_return_val;
int enable_return_val;
int disable_return_val;
int is_pending_return_val;
int configure_return_val;
int set_pending_return_val;
int clear_pending_return_val;
int set_isr_return_val;
int set_isr_param_return_val;
int set_isr_nmi_return_val;
int set_isr_nmi_param_return_val;
int set_isr_fault_return_val;
int get_current_return_val;
int set_intr_priority_return_val;

void fake_isr(void)
{
    return;
}

void fake_isr_param(uintptr_t param)
{
    return;
}

int arch_interrupt_init(void)
{
    return init_return_val;
}

int arch_interrupt_is_enabled(unsigned int interrupt, bool *state)
{
    return is_enabled_return_val;
}

int arch_interrupt_enable(unsigned int interrupt)
{
    return enable_return_val;
}

int arch_interrupt_disable(unsigned int interrupt)
{
    return disable_return_val;
}

int arch_interrupt_is_pending(unsigned int interrupt, bool *state)
{
    return is_pending_return_val;
}

int arch_interrupt_configure(unsigned int interrupt, unsigned int cfg)
{
    return configure_return_val;
}

int arch_interrupt_set_priority(unsigned int interrupt, unsigned int cfg)
{
    return set_intr_priority_return_val;
}

int arch_interrupt_set_pending(unsigned int interrupt)
{
    return set_pending_return_val;
}

int arch_interrupt_clear_pending(unsigned int interrupt)
{
    return clear_pending_return_val;
}

int arch_interrupt_set_isr(unsigned int interrupt, void (*isr)(void))
{
    return set_isr_return_val;
}

int arch_interrupt_set_isr_param(
    unsigned int interrupt,
    void (*isr)(uintptr_t p),
    uintptr_t p)
{
    return set_isr_param_return_val;
}

int arch_interrupt_set_isr_nmi(void (*isr)(void))
{
    return set_isr_nmi_return_val;
}

int arch_interrupt_set_isr_irq(unsigned int interrupt, void (*isr)(void))
{
    return set_isr_return_val;
}

int arch_interrupt_set_isr_irq_param(
    unsigned int interrupt,
    void (*isr)(uintptr_t param),
    uintptr_t parameter)
{
    return set_isr_param_return_val;
}

int arch_interrupt_set_isr_nmi_param(void (*isr)(uintptr_t p), uintptr_t p)
{
    return set_isr_nmi_param_return_val;
}

int arch_interrupt_set_isr_fault(void (*isr)(void))
{
    return set_isr_fault_return_val;
}

int arch_interrupt_get_current(unsigned int *interrupt)
{
    return get_current_return_val;
}

bool arch_interrupt_is_interrupt_context(void)
{
    return (get_current_return_val == FWK_SUCCESS);
}
