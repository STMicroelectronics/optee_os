/*
 * Arm SCP/MCP Software
 * Copyright (c) 2024-2025, Arm Limited and Contributors. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

.macro ventry label
.align	7
b \label
.endm

.align 11
.global exception_vectors
exception_vectors:
    /* current EL, SP_EL0 */
    ventry err_exception    /* synchronous */
    ventry err_exception    /* IRQ */
    ventry err_exception    /* FIQ */
    ventry err_exception    /* SError */

    /* current EL, SP_ELX */
    ventry err_exception    /* synchronous */
    ventry err_exception    /* IRQ */
    ventry fiq_handler      /* FIQ */
    ventry err_exception    /* SError */

    /* lower EL using AArch64 */
    ventry err_exception    /* synchronous */
    ventry err_exception    /* IRQ */
    ventry err_exception    /* FIQ */
    ventry err_exception    /* SError */

    /* lower EL using AArch32 */
    ventry err_exception    /* synchronous */
    ventry err_exception    /* IRQ */
    ventry err_exception    /* FIQ */
    ventry err_exception    /* SError */

err_exception:
    wfe
    b err_exception

fiq_handler:
    /* Save registers onto stack */
    str x30, [sp, #-16]!
    stp x28, x29, [sp, #-16]!
    stp x26, x27, [sp, #-16]!
    stp x24, x25, [sp, #-16]!
    stp x22, x23, [sp, #-16]!
    stp x20, x21, [sp, #-16]!
    stp x18, x19, [sp, #-16]!
    stp x16, x17, [sp, #-16]!
    stp x14, x15, [sp, #-16]!
    stp x12, x13, [sp, #-16]!
    stp x10, x11, [sp, #-16]!
    stp x8, x9, [sp, #-16]!
    stp x6, x7, [sp, #-16]!
    stp x4, x5, [sp, #-16]!
    stp x2, x3, [sp, #-16]!
    stp x0, x1, [sp, #-16]!

    /* Save exception state on stack to support nested exceptions */
    mrs x0, spsr_el2
    mrs x1, elr_el2
    stp x0, x1, [sp, #-16]!

.extern irq_global
    bl irq_global

    /* Restore exception state */
    ldp x0, x1, [sp], #16
    msr spsr_el2, x0
    msr elr_el2, x1

    /* Restore registers from stack */
    ldp x0, x1, [sp], #16
    ldp x2, x3, [sp], #16
    ldp x4, x5, [sp], #16
    ldp x6, x7, [sp], #16
    ldp x8, x9, [sp], #16
    ldp x10, x11, [sp], #16
    ldp x12, x13, [sp], #16
    ldp x14, x15, [sp], #16
    ldp x16, x17, [sp], #16
    ldp x18, x19, [sp], #16
    ldp x20, x21, [sp], #16
    ldp x22, x23, [sp], #16
    ldp x24, x25, [sp], #16
    ldp x26, x27, [sp], #16
    ldp x28, x29, [sp], #16
    ldr x30, [sp], #16

    eret
