/* SPDX-License-Identifier: BSD-2-Clause */
/*
 * Copyright (c) 2024, STMicroelectronics
 */

#ifndef __PM_STM32MP1_PSCI_H
#define __PM_STM32MP1_PSCI_H

#include <sm/sm.h>
#include <stdint.h>

/* STM32MP1 PSCI system suspend entry, possibly excluded from unpaged memory */
uint32_t __psci_system_suspend(uintptr_t entry, uint32_t context_id,
			       struct sm_nsec_ctx *nsec);

/* STM32MP1 PSCI system off entry, possibly excluded from unpaged memory */
void __psci_system_off(void);

#endif /*__PM_STM32MP1_PSCI_H*/
