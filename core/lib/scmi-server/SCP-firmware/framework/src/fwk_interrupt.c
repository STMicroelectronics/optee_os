/*
 * Arm SCP/MCP Software
 * Copyright (c) 2015-2025, Arm Limited and Contributors. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 * Description:
 *     Interrupt management.
 */

#include <internal/fwk_interrupt.h>

#include <fwk_arch.h>
#include <fwk_interrupt.h>
#include <fwk_status.h>

#include <arch_interrupt.h>

int fwk_arch_interrupt_init(void)
{
    return arch_interrupt_init();
}

int fwk_interrupt_is_enabled(unsigned int interrupt, bool *enabled)
{
    if (enabled == NULL) {
        return FWK_E_PARAM;
    }

    return arch_interrupt_is_enabled(interrupt, enabled);
}

int fwk_interrupt_enable(unsigned int interrupt)
{
    return arch_interrupt_enable(interrupt);
}

int fwk_interrupt_disable(unsigned int interrupt)
{
    return arch_interrupt_disable(interrupt);
}

int fwk_interrupt_is_pending(unsigned int interrupt, bool *pending)
{
    if (pending == NULL) {
        return FWK_E_PARAM;
    }

    return arch_interrupt_is_pending(interrupt, pending);
}

int fwk_interrupt_configure(unsigned int interrupt, unsigned int cfg)
{
    return arch_interrupt_configure(interrupt, cfg);
}

int fwk_interrupt_set_pending(unsigned int interrupt)
{
    return arch_interrupt_set_pending(interrupt);
}

int fwk_interrupt_clear_pending(unsigned int interrupt)
{
    return arch_interrupt_clear_pending(interrupt);
}

int fwk_interrupt_set_priority(unsigned int interrupt, unsigned int val)
{
    return arch_interrupt_set_priority(interrupt, val);
}

int fwk_interrupt_set_isr(unsigned int interrupt, void (*isr)(void))
{
    if (isr == NULL) {
        return FWK_E_PARAM;
    }

    if (interrupt == FWK_INTERRUPT_NMI) {
        return arch_interrupt_set_isr_nmi(isr);
    } else {
        return arch_interrupt_set_isr_irq(interrupt, isr);
    }
}

int fwk_interrupt_set_isr_param(unsigned int interrupt,
                                void (*isr)(uintptr_t param),
                                uintptr_t param)
{
    if (isr == NULL) {
        return FWK_E_PARAM;
    }

    if (interrupt == FWK_INTERRUPT_NMI) {
        return arch_interrupt_set_isr_nmi_param(isr, param);
    } else {
        return arch_interrupt_set_isr_irq_param(interrupt, isr, param);
    }
}

int fwk_interrupt_get_current(unsigned int *interrupt)
{
    if (interrupt == NULL) {
        return FWK_E_PARAM;
    }

    return arch_interrupt_get_current(interrupt);
}

bool fwk_is_interrupt_context(void)
{
    return arch_interrupt_is_interrupt_context();
}

/* This function is only for internal use by the framework */
int fwk_interrupt_set_isr_fault(void (*isr)(void))
{
    if (isr == NULL) {
        return FWK_E_PARAM;
    }

    return arch_interrupt_set_isr_fault(isr);
}
