// SPDX-License-Identifier: BSD-2-Clause
/*
 * Copyright (c) 2022, Linaro Limited
 * Copyright (c) 2024, STMicroelectronics
 */
#include <compiler.h>
#include <drivers/clk.h>
#include <kernel/panic.h>
#include <pm/stm32mp1_psci.h>
#include <stm32_util.h>

const struct clk_ops stm32mp1_clk_ops __rodata_dummy;

#ifdef CFG_PAGED_PSCI_SYSTEM_SUSPEND
uint32_t __section(".text.dummy.__psci_system_suspend")
__psci_system_suspend(uintptr_t entry __unused,
		      uint32_t context_id __unused,
		      struct sm_nsec_ctx *nsec __unused)
{
	return 0;
}
#endif

#ifdef CFG_PAGED_PSCI_SYSTEM_OFF
void __section(".text.dummy.__psci_system_off") __noreturn
__psci_system_off(void)
{
	panic();
}
#endif
