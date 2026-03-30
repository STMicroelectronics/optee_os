/*
 * Arm SCP/MCP Software
 * Copyright (c) 2015-2025, Arm Limited and Contributors. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 * Description:
 *     Interrupt management.
 */

#include <fwk_arch.h>
#include <fwk_status.h>

#include <stdbool.h>
#include <stdint.h>

int arch_interrupt_is_enabled(unsigned int interrupt, bool *state)
{
    return FWK_E_SUPPORT;
}

int arch_interrupt_enable(unsigned int interrupt)
{
    return FWK_E_SUPPORT;
}

int arch_interrupt_disable(unsigned int interrupt)
{
    return FWK_E_SUPPORT;
}

int arch_interrupt_is_pending(unsigned int interrupt, bool *state)
{
    return FWK_E_SUPPORT;
}

int arch_interrupt_configure(unsigned int interrupt, unsigned int cfg)
{
    return FWK_E_SUPPORT;
}

int arch_interrupt_set_pending(unsigned int interrupt)
{
    return FWK_E_SUPPORT;
}

int arch_interrupt_set_priority(unsigned int interrupt, unsigned int val)
{
    return FWK_E_SUPPORT;
}

int arch_interrupt_clear_pending(unsigned int interrupt)
{
    return FWK_E_SUPPORT;
}

int arch_interrupt_set_isr_irq(unsigned int interrupt, void (*isr)(void))
{
    return FWK_E_SUPPORT;
}

int arch_interrupt_set_isr_irq_param(
    unsigned int interrupt,
    void (*isr)(uintptr_t param),
    uintptr_t parameter)
{
    return FWK_E_SUPPORT;
}

int arch_interrupt_set_isr_nmi(void (*isr)(void))
{
    return FWK_E_SUPPORT;
}

int arch_interrupt_set_isr_nmi_param(
    void (*isr)(uintptr_t param),
    uintptr_t parameter)
{
    return FWK_E_SUPPORT;
}

int arch_interrupt_set_isr_fault(void (*isr)(void))
{
    return FWK_E_SUPPORT;
}

int arch_interrupt_get_current(unsigned int *interrupt)
{
    return FWK_E_SUPPORT;
}

bool arch_interrupt_is_interrupt_context(void)
{
    return false;
}

int arch_interrupt_init(void)
{
    return FWK_SUCCESS;
}
