/*
 * Arm SCP/MCP Software
 * Copyright (c) 2022-2025, Arm Limited and Contributors. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef ARCH_INTERRUPT_H
#define ARCH_INTERRUPT_H

/*!
 * \brief Suspend execution of current CPU.
 *
 */
static inline void arch_suspend(void)
{
}

/*
 * This variable is used to ensure spurious nested calls won't
 * enable interrupts. This is been defined in fwk_test.c
 */

extern unsigned int critical_section_nest_level;

static inline void arch_interrupt_global_enable(unsigned int not_used)
{
    /* Decrement critical_section_nest_level only if in critical section */
    if (critical_section_nest_level > 0) {
        critical_section_nest_level--;
    }
}

static inline unsigned int arch_interrupt_global_disable(void)
{
    critical_section_nest_level++;

    return 0;
}

int arch_interrupt_init(void);

#endif /* ARCH_INTERRUPT_H */
