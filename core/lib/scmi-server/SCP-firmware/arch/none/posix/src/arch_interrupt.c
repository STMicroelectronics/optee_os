/*
 * Arm SCP/MCP Software
 * Copyright (c) 2025, Arm Limited and Contributors. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 * Description:
 *     Interrupt management.
 */

#include <fwk_arch.h>
#include <fwk_assert.h>
#include <fwk_status.h>

#include <signal.h>
#include <stdbool.h>
#include <stdint.h>
#include <string.h>
#include <time.h>

#define IRQ_NUM (0x100)

struct arch_irq {
    union {
        void (*callback)(void);
        void (*callback_param)(uintptr_t);
        void *raw;
    } isr;

    uintptr_t param;
    bool use_param;
    bool is_enabled;
    bool is_pending;
};

static struct arch_interrupt_ctx {
    struct arch_irq irq_table[IRQ_NUM];
    int current_interrupt;
} arch_interrupt_ctx;

static void handle_host_signal(int sig, siginfo_t *info, void *ucontext)
{
    struct arch_irq *irq;
    irq = &arch_interrupt_ctx.irq_table[info->si_value.sival_int];

    if (!fwk_expect(irq->is_enabled == true)) {
        return;
    }

    arch_interrupt_ctx.current_interrupt = info->si_value.sival_int;
    if (irq->use_param) {
        irq->isr.callback_param(irq->param);
    } else {
        irq->isr.callback();
    }
    arch_interrupt_ctx.current_interrupt = IRQ_NUM;
}

int arch_interrupt_is_enabled(unsigned int interrupt, bool *state)
{
    return FWK_E_SUPPORT;
}

int arch_interrupt_enable(unsigned int interrupt)
{
    struct arch_irq *irq;
    if (interrupt >= IRQ_NUM) {
        return FWK_E_PARAM;
    }

    irq = &arch_interrupt_ctx.irq_table[interrupt];
    if (irq->is_enabled) {
        return FWK_E_PARAM;
    }

    irq->is_enabled = true;
    return FWK_SUCCESS;
}

int arch_interrupt_disable(unsigned int interrupt)
{
    struct arch_irq *irq;
    if (interrupt >= IRQ_NUM) {
        return FWK_E_PARAM;
    }

    irq = &arch_interrupt_ctx.irq_table[interrupt];
    irq->is_enabled = false;
    return FWK_SUCCESS;
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
    if (interrupt >= IRQ_NUM) {
        return FWK_E_PARAM;
    }
    struct arch_irq *irq;
    irq = &arch_interrupt_ctx.irq_table[interrupt];
    irq->is_pending = false;
    return FWK_SUCCESS;
}

int arch_interrupt_set_isr_irq(unsigned int interrupt, void (*isr)(void))
{
    struct arch_irq *irq;
    if (interrupt >= IRQ_NUM) {
        return FWK_E_PARAM;
    }

    irq = &arch_interrupt_ctx.irq_table[interrupt];
    irq = &arch_interrupt_ctx.irq_table[interrupt];
    irq->use_param = false;
    irq->isr.callback = isr;

    return FWK_SUCCESS;
}

int arch_interrupt_set_isr_irq_param(
    unsigned int interrupt,
    void (*isr)(uintptr_t param),
    uintptr_t parameter)
{
    struct arch_irq *irq;
    if (interrupt >= IRQ_NUM) {
        return FWK_E_PARAM;
    }

    irq = &arch_interrupt_ctx.irq_table[interrupt];
    irq->param = parameter;
    irq->use_param = true;
    irq->isr.callback_param = isr;

    return FWK_SUCCESS;
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
    *interrupt = arch_interrupt_ctx.current_interrupt;
    if (*interrupt == IRQ_NUM) {
        return FWK_E_STATE;
    }
    return FWK_SUCCESS;
}

bool arch_interrupt_is_interrupt_context(void)
{
    return false;
}

int arch_interrupt_init(void)
{
    memset(
        arch_interrupt_ctx.irq_table, 0, sizeof(arch_interrupt_ctx.irq_table));

    struct sigaction sa;
    memset(&sa, 0, sizeof(sa));
    sa.sa_sigaction = handle_host_signal;
    sa.sa_flags = SA_SIGINFO;
    sigaction(SIGUSR1, &sa, NULL);

    return FWK_SUCCESS;
}
