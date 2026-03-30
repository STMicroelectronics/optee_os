/*
 * Arm SCP/MCP Software
 * Copyright (c) 2022-2025, Linaro Limited and Contributors. All rights
 * reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef ARCH_INTERRUPT_H
#define ARCH_INTERRUPT_H

/*!
 * \brief Enables global CPU interrupts. (stub)
 *
 */
static inline void arch_interrupt_global_enable(unsigned int not_used)
{
}

/*!
 * \brief Disables global CPU interrupts. (stub)
 *
 */
static inline unsigned int arch_interrupt_global_disable(void)
{
    return 0;
}

/*!
 * \brief Suspend execution of current CPU.
 *
 */
static inline void arch_suspend(void)
{
}

/*!
 * \brief Initialize the architecture interrupt management component.
 *
 * \param[out] Pointer to the interrupt driver.
 *
 * \retval ::FWK_E_PANIC The operation failed.
 * \retval ::FWK_SUCCESS The operation succeeded.
 *
 * \return Status code representing the result of the operation.
 */

#endif /* ARCH_INTERRUPT_H */
