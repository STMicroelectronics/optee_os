/*
 * Arm SCP/MCP Software
 * Copyright (c) 2015-2025, Arm Limited and Contributors. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 * Description:
 *     Interrupt management.
 */

#ifndef FWK_INTERRUPT_H
#define FWK_INTERRUPT_H

#include <fwk_arch.h>
#include <fwk_macros.h>
#include <fwk_status.h>

#include <arch_interrupt.h>

#include <limits.h>
#include <stdbool.h>
#include <stdint.h>

/*!
 * \addtogroup GroupLibFramework Framework
 * \{
 */

/*!
 * \defgroup GroupInterrupt Interrupt Management
 * \{
 */

/*!
 * \brief Non-maskable interrupt identifier.
 *
 * \note This identifier can only be used in specific functions of this API.
 *      Please refer to the individual function documentation for more details.
 */
#define FWK_INTERRUPT_NMI UINT_MAX

/*!
 * \brief Non-existing interrupt identifier.
 *
 * \details Non-existing interrupt identifier to be used by configuration data
 *      when an interrupt identifier is expected but the system does not support
 *      it.
 *
 * \note This identifier cannot be used with the Interrupt Management component
 *      API. Its intention is to provide a standard way to describe an
 *      unconnected (or invalid) IRQ to the drivers.
 */
#define FWK_INTERRUPT_NONE (UINT_MAX - 1)

/*!
 * \brief Exception identifier.
 *
 * \details Exception interrupt identifier.
 *
 * \note This identifier can only be used in specific functions of this API.
 *      Please refer to the individual function documentation for more details.
 */
#define FWK_INTERRUPT_EXCEPTION (UINT_MAX - 2)

/*!
 * \brief Enable interrupts.
 *
 * \param flags Value returned by fwk_interrupt_global_disable.
 */
static inline void fwk_interrupt_global_enable(unsigned int flags)
{
    arch_interrupt_global_enable(flags);
}

/*!
 * \brief Disable interrupts.
 *
 * \retval Opaque value to be passed when enabling.
 */
static inline unsigned int fwk_interrupt_global_disable(void)
{
    return arch_interrupt_global_disable();
}

/*!
 * \brief Initialize the architecture interrupt management.
 *
 * \retval ::FWK_SUCCESS Operation succeeded.
 * \retval ::FWK_E_PANIC The operation failed.
 */
int fwk_arch_interrupt_init(void);

/*!
 * \brief Test whether an interrupt is enabled.
 *
 * \param interrupt Interrupt number.
 * \param [out] enabled \c true if the interrupt is enabled, else \c false.
 *
 * \retval ::FWK_SUCCESS Operation succeeded.
 * \retval ::FWK_E_PARAM One or more parameters were invalid.
 * \retval ::FWK_E_INIT The component has not been initialized.
 */
int fwk_interrupt_is_enabled(unsigned int interrupt, bool *enabled);

/*!
 * \brief Enable interrupt.
 *
 * \param interrupt Interrupt number.
 *
 * \retval ::FWK_SUCCESS Operation succeeded.
 * \retval ::FWK_E_PARAM One or more parameters were invalid.
 * \retval ::FWK_E_INIT The component has not been initialized.
 */
int fwk_interrupt_enable(unsigned int interrupt);

/*!
 * \brief Disable interrupt.
 *
 * \param interrupt Interrupt number.
 *
 * \retval ::FWK_SUCCESS Operation succeeded.
 * \retval ::FWK_E_PARAM One or more parameters were invalid.
 * \retval ::FWK_E_INIT The component has not been initialized.
 */
int fwk_interrupt_disable(unsigned int interrupt);

/*!
 * \brief Check if an interrupt is pending.
 *
 * \param interrupt Interrupt number.
 * \param [out] pending \c true if the interrupt is pending, else \c false.
 *
 * \retval ::FWK_SUCCESS Operation succeeded.
 * \retval ::FWK_E_PARAM One or more parameters were invalid.
 * \retval ::FWK_E_INIT The component has not been initialized.
 */
int fwk_interrupt_is_pending(unsigned int interrupt, bool *pending);

/*!
 * \brief Configure Interrupt
 *
 * \param interrupt Interrupt number.
 * \param cfg       Interrupt configuration value.
 *
 * \retval ::FWK_SUCCESS Operation succeeded.
 * \retval ::FWK_E_SUPPORT Operation is not supported yet.
 */
int fwk_interrupt_configure(unsigned int interrupt, unsigned int cfg);

/*!
 * \brief Change interrupt status to pending.
 *
 * \param interrupt Interrupt number.
 *
 * \retval ::FWK_SUCCESS Operation succeeded.
 * \retval ::FWK_E_PARAM One or more parameters were invalid.
 * \retval ::FWK_E_INIT The component has not been initialized.
 */
int fwk_interrupt_set_pending(unsigned int interrupt);

/*!
 * \brief Clear the interrupt pending status.
 *
 * \param interrupt Interrupt number.
 *
 * \retval ::FWK_SUCCESS Operation succeeded.
 * \retval ::FWK_E_PARAM One or more parameters were invalid.
 * \retval ::FWK_E_INIT The component has not been initialized.
 */
int fwk_interrupt_clear_pending(unsigned int interrupt);

/*!
 * \brief Set interrupt priority.
 *
 * \param interrupt Interrupt number.
 * \param val Interrupt priority value, dependant on the interrupt controller
 *
 * \retval ::FWK_SUCCESS Operation succeeded.
 * \retval ::FWK_E_PARAM One or more parameters were invalid.
 */
int fwk_interrupt_set_priority(unsigned int interrupt, unsigned int val);

/*!
 * \brief Assign an interrupt service routine to an interrupt.
 *
 * \param interrupt Interrupt number. This function accepts FWK_INTERRUPT_NMI as
 *      an interrupt number.
 * \param isr Pointer to the interrupt service routine function.
 *
 * \retval ::FWK_SUCCESS Operation succeeded.
 * \retval ::FWK_E_PARAM One or more parameters were invalid.
 * \retval ::FWK_E_INIT The component has not been initialized.
 */
int fwk_interrupt_set_isr(unsigned int interrupt, void (*isr)(void));

/*!
 * \brief Assign an interrupt service routine that receives a parameter to an
 *     interrupt.
 *
 * \param interrupt Interrupt number. This function accepts FWK_INTERRUPT_NMI as
 *      an interrupt number.
 * \param isr Pointer to the interrupt service routine function.
 * \param param Parameter that should be passed to the isr when it is called.
 *
 * \retval ::FWK_SUCCESS Operation succeeded.
 * \retval ::FWK_E_PARAM One or more parameters were invalid.
 * \retval ::FWK_E_INIT The component has not been initialized.
 */
int fwk_interrupt_set_isr_param(unsigned int interrupt,
                                void (*isr)(uintptr_t param),
                                uintptr_t param);

/*!
 * \brief Get the interrupt number for the interrupt service routine being
 *      processed.
 *
 * \param [out] interrupt Interrupt number. This function may return
 *      also:
 *          * FWK_INTERRUPT_NMI - When the current interrupt is an NMI.
 *          * FWK_INTERRUPT_EXCEPTION - When the current interrupt is an
 *            exception.
 *
 * \retval ::FWK_SUCCESS Operation succeeded.
 * \retval ::FWK_E_PARAM One or more parameters were invalid.
 * \retval ::FWK_E_STATE An interrupt is not currently being serviced.
 * \retval ::FWK_E_INIT The component has not been initialized.
 */
int fwk_interrupt_get_current(unsigned int *interrupt);

/*!
 * \brief Check if in interrupt context.
 *
 * \retval true if in an interrupt context.
 * \retval false not in an interrupt context.
 */
bool fwk_is_interrupt_context(void);

/*!
 * \}
 */

/*!
 * \}
 */

#endif /* FWK_INTERRUPT_H */
