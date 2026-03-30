/*
 * Arm SCP/MCP Software
 * Copyright (c) 2020-2025, Arm Limited and Contributors. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef ARCH_NVIC_H
#define ARCH_NVIC_H

#include <fwk_arch.h>

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
int arch_interrupt_init(void);

#endif /* ARCH_NVIC_H */
