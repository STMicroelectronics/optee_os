/*
 * Arm SCP/MCP Software
 * Copyright (c) 2024-2025, Arm Limited and Contributors. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <arch_reg.h>
#include <arch_mem_mode.h>

    .section .entrypoint, "ax"

#ifdef USE_NEWLIB
    .extern __libc_init_array
#endif
    .extern arm_main

    .global arch_exception_reset;
arch_exception_reset:
    /* Check current exception level */
    mrs x0, CurrentEL
    cmp x0, (2 << 2)
    b.gt el3_init
    b.eq el2_init
    b wfe_loop

el3_init:
#if defined(ARMV8A64_EL3_SUPPORT)
    /* switch to el2 */
    msr SCTLR_EL2, xzr
    msr HCR_EL2, xzr

    mrs x0, SCR_EL3
    orr x0, x0, #(SCR_EL3_RW) /* Next lower exception level is AArch64 */
    msr SCR_EL3, x0

    mrs x0, SPSR_EL3
    orr x0, x0, #(SPSR_EL3_A)  /* Mask SError */
    orr x0, x0, #(SPSR_EL3_I)  /* Mask IRQ */
    orr x0, x0, #(SPSR_EL3_F)  /* Mask FIQ */
    mov x1, #(SPSR_EL3_M_EL2H) /* EL2 is in handler mode */
    orr x0, x0, x1
    msr SPSR_EL3, x0

    ldr x0, =SCTLR_EL3_RESET
    orr x0, x0, #(SCTLR_EL3_NAA)
    msr SCTLR_EL3, x0

    /* Configure EL3 exception vector for managing FIQ */
    ldr x0, =exception_vectors
    msr vbar_el3, x0
    isb

    adr x0, el2_init
    msr ELR_EL3, x0

    eret
#else
    b wfe_loop
#endif /* BUILD_HAS_AARCH64_ARMV8R64 */

el2_init:
    /* Configure exception vectors */
    ldr x0, =exception_vectors
    msr vbar_el2, x0
    isb

    /* Initialize HCR */
    ldr x0, =HCR_EL2_RESET
    msr hcr_el2, x0

    /* Initialize SCTLR with PMSA disabled */
    ldr x0, =SCTLR_EL2_RESET
    msr sctlr_el2, x0

    dsb sy
    isb

    /* Ensure PMSA is present */
    mrs x0, id_aa64mmfr0_el1
    and x1, x0, #ID_AA64MMFR0_EL1_MSA_MASK
    cbz x1, no_pmsa_support
    and x1, x0, #ID_AA64MMFR0_EL1_MSA_FRAC_MASK
    cbz x1, no_pmsa_support


    /* Initial MPU region config */
#if FMW_MEM_MODE == ARCH_MEM_MODE_SINGLE_REGION
    /* Region 0 */
    ldr x1, =PRBAR_VALUE(FMW_MEM0_BASE, PRBAR_SH_NON_SHAREABLE, PRBAR_AP_RW_EL2, PRBAR_XN_PERMITTED)
    ldr x2, =PRLAR_VALUE(FMW_MEM0_BASE + FMW_MEM0_SIZE - 1, PRLAR_NS_SECURE, 0, PRLAR_EN_ENABLED)
    /* Region 1 */
    mov x3, #0
    mov x4, #0
#else
    /* Region 0 */
    ldr x1, =PRBAR_VALUE(FMW_MEM0_BASE, PRBAR_SH_NON_SHAREABLE, PRBAR_AP_RO_EL2, PRBAR_XN_PERMITTED)
    ldr x2, =PRLAR_VALUE(FMW_MEM0_BASE + FMW_MEM0_SIZE - 1, PRLAR_NS_SECURE, 0, PRLAR_EN_ENABLED)
    /* Region 1 */
    ldr x3, =PRBAR_VALUE(FMW_MEM1_BASE, PRBAR_SH_NON_SHAREABLE, PRBAR_AP_RW_EL2, PRBAR_XN_NOT_PERMITTED)
    ldr x4, =PRLAR_VALUE(FMW_MEM1_BASE + FMW_MEM1_SIZE - 1, PRLAR_NS_SECURE, 0, PRLAR_EN_ENABLED)
#endif

    mov x0, MAIR_NORMAL_WB_NT
    msr mair_el2, x0

    msr prselr_el2, xzr
    isb
    msr prbar_el2, x1
    msr prlar_el2, x2
    dsb sy
    mov x0, #1
    msr prselr_el2, x0
    isb
    msr prbar_el2, x3
    msr prlar_el2, x4
    dsb sy
    isb

no_pmsa_support:

    /* Enable PMSA/MMU, data cache and instruction cache */
    ldr x0, =SCTLR_EL2_RESET
    mov x1, #(SCTLR_EL2_M | SCTLR_EL2_C | SCTLR_EL2_I)
    orr x0, x0, x1
    msr sctlr_el2, x0
    dsb sy
    isb

    /* Zero BSS */
    ldr x0, =__bss_start__
    ldr x1, =__bss_end__
    cmp x0, x1
    beq skip_bss

1:  str xzr, [x0], #8
    cmp x0, x1
    b.lo 1b

skip_bss:
    /* Setup stack */
    ldr x0, =stack_top
    mov	sp, x0

#ifdef USE_NEWLIB
    /* Call FWK_CONSTRUCTOR functions */
    bl __libc_init_array
#elif defined(BUILD_HAS_NOTIFICATION)
    bl  __fwk_notification_reset
#endif

    bl  arm_main

wfe_loop:
    wfe
    b wfe_loop

    .section .bss
    .align 4;
    .global stack_bottom;
stack_bottom:
    .zero FMW_STACK_SIZE
    .global stack_top;
stack_top:
