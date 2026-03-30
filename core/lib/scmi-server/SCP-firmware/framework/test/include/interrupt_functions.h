/*
 * Arm SCP/MCP Software
 * Copyright (c) 2025, Arm Limited and Contributors. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef INTERRUPT_FUNCTIONS_H_
#define INTERRUPT_FUNCTIONS_H_

#include <fwk_status.h>

#include <stdint.h>

extern int init_return_val;
extern int is_enabled_return_val;
extern int enable_return_val;
extern int disable_return_val;
extern int is_pending_return_val;
extern int set_pending_return_val;
extern int clear_pending_return_val;
extern int set_isr_return_val;
extern int set_isr_param_return_val;
extern int set_isr_nmi_return_val;
extern int set_isr_nmi_param_return_val;
extern int set_isr_fault_return_val;
extern int get_current_return_val;
extern int configure_return_val;
extern int set_intr_priority_return_val;

void fake_isr(void);
void fake_isr_param(uintptr_t param);
void test_case_setup(void);

#endif /* INTERRUPT_FUNCTIONS_H_ */
