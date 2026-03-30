/*
 * Arm SCP/MCP Software
 * Copyright (c) 2024-2025, Arm Limited and Contributors. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <fwk_arch.h>
#include <fwk_assert.h>
#include <fwk_core.h>
#include <fwk_mmio.h>

#include <arch_interrupt.h>
#include <arch_reg.h>

#include <fmw_gic.h>

#define INTERRUPT_ID_SGI_LIMIT 16
#define INTERRUPT_ID_PPI_LIMIT 32
#define INTERRUPT_ID_SPI_LIMIT 1020
#define INTERRUPT_ID_ISR_LIMIT INTERRUPT_ID_SPI_LIMIT
#define INTERRUPT_ID_INVALID   1024

#define GIC_CFG_BITS_PER_INT 2U /* Each interrupt uses 2 bits */
#define GIC_CFG_MASK         0x3U /* Mask for one interrupt's config */
#define GIC_PPI_BASE         16U /* PPIs start at interrupt 16 */
#define GIC_INTS_PER_REG     16U /* Each ICFGR register covers 16 interrupts */

#define GIC_INTS_PER_REG_SHIFT 4U
#define GIC_INTS_PER_REG_MASK  (GIC_INTS_PER_REG - 1U)
#define GIC_BITS_PER_INT_SHIFT 1U

enum interrupt_type {
    INTERRUPT_TYPE_SGI,
    INTERRUPT_TYPE_PPI,
    INTERRUPT_TYPE_SPI,
    INTERRUPT_TYPE_OTHER,
};

static enum interrupt_type interrupt_type_from_id(unsigned int interrupt)
{
    if (interrupt < INTERRUPT_ID_SGI_LIMIT) {
        return INTERRUPT_TYPE_SGI;
    } else if (interrupt < INTERRUPT_ID_PPI_LIMIT) {
        return INTERRUPT_TYPE_PPI;
    } else if (interrupt < INTERRUPT_ID_SPI_LIMIT) {
        return INTERRUPT_TYPE_SPI;
    } else {
        return INTERRUPT_TYPE_OTHER;
    }
}

struct isr_callback {
    union {
        void (*func)();
        void (*func_with_param)(uintptr_t);
    };
    uintptr_t param;
    bool use_param;
};

static unsigned int current_iar = INTERRUPT_ID_INVALID;
static struct isr_callback callback[INTERRUPT_ID_ISR_LIMIT] = { 0 };

#ifndef GIC_V2
static uint64_t read_icc_iar0_el1(void)
{
    return READ_SYSREG(icc_iar_el1);
}

static void write_icc_eoir0_el1(uint64_t value)
{
    return WRITE_SYSREG(icc_eoir0_el1, value);
}
#endif

void irq_global(void)
{
    struct isr_callback *entry;
    uint32_t iar;

#ifdef GIC_V2
    iar = fwk_mmio_read_32(FMW_GICD_BASE + GICC_BASE + GICC_IAR);
    fwk_mmio_write_32(FMW_GICD_BASE + GICC_BASE + GICC_EOIR, iar);
    iar = iar & 0x00000FFFUL;
#else
    iar = read_icc_iar0_el1();
#endif

    current_iar = iar;

    if (iar >= INTERRUPT_ID_ISR_LIMIT) {
        return;
    }

    entry = &callback[iar];

    if (entry->use_param == false) {
        if (entry->func != NULL) {
            entry->func();
        }
    } else {
        if (entry->func_with_param != NULL) {
            entry->func_with_param(entry->param);
        }
    }

#ifndef GIC_V2
    write_icc_eoir0_el1(iar);
#endif
    current_iar = INTERRUPT_ID_INVALID;
}

int arch_interrupt_is_enabled(unsigned int interrupt, bool *enabled)
{
    switch (interrupt_type_from_id(interrupt)) {
    case INTERRUPT_TYPE_SGI:
    case INTERRUPT_TYPE_PPI:
        *enabled =
            (fwk_mmio_read_32(FMW_GICR_BASE + GICR_SGI_BASE + GICR_ISENABLER0) &
             FWK_BIT(interrupt)) != 0;
        break;
    case INTERRUPT_TYPE_SPI:
        *enabled =
            (fwk_mmio_read_32(FMW_GICD_BASE + GICD_ISENABLER(interrupt / 32)) &
             FWK_BIT(interrupt % 32)) != 0;
        break;
    case INTERRUPT_TYPE_OTHER:
        return FWK_E_SUPPORT;
    }

    return FWK_SUCCESS;
}

int arch_interrupt_enable(unsigned int interrupt)
{
    switch (interrupt_type_from_id(interrupt)) {
    case INTERRUPT_TYPE_SGI:
    case INTERRUPT_TYPE_PPI:
        fwk_mmio_write_32(
            FMW_GICR_BASE + GICR_SGI_BASE + GICR_ISENABLER0,
            FWK_BIT(interrupt));
        break;
    case INTERRUPT_TYPE_SPI:
        fwk_mmio_write_32(
            FMW_GICD_BASE + GICD_ISENABLER(interrupt / 32),
            FWK_BIT(interrupt % 32));
        break;
    case INTERRUPT_TYPE_OTHER:
        return FWK_E_SUPPORT;
    }

    return FWK_SUCCESS;
}

int arch_interrupt_disable(unsigned int interrupt)
{
    switch (interrupt_type_from_id(interrupt)) {
    case INTERRUPT_TYPE_SGI:
    case INTERRUPT_TYPE_PPI:
        fwk_mmio_write_32(
            FMW_GICR_BASE + GICR_SGI_BASE + GICR_ICENABLER0,
            FWK_BIT(interrupt));
        break;
    case INTERRUPT_TYPE_SPI:
        fwk_mmio_write_32(
            FMW_GICD_BASE + GICD_ICENABLER(interrupt / 32),
            FWK_BIT(interrupt % 32));
        break;
    case INTERRUPT_TYPE_OTHER:
        return FWK_E_SUPPORT;
    }

    return FWK_SUCCESS;
}

int arch_interrupt_is_pending(unsigned int interrupt, bool *pending)
{
    switch (interrupt_type_from_id(interrupt)) {
    case INTERRUPT_TYPE_SGI:
    case INTERRUPT_TYPE_PPI:
        fwk_mmio_write_32(
            FMW_GICR_BASE + GICR_SGI_BASE + GICR_ICENABLER0,
            FWK_BIT(interrupt));
        break;
    case INTERRUPT_TYPE_SPI:
        fwk_mmio_write_32(
            FMW_GICD_BASE + GICD_ICENABLER(interrupt / 32),
            FWK_BIT(interrupt % 32));
        break;
    case INTERRUPT_TYPE_OTHER:
        return FWK_E_SUPPORT;
    }

    return FWK_SUCCESS;
}

int arch_interrupt_configure(unsigned int interrupt, unsigned int cfg)
{
    unsigned int shift, n;
    uint32_t val;
    uintptr_t reg_addr;

    switch (interrupt_type_from_id(interrupt)) {
    case INTERRUPT_TYPE_SGI:
        reg_addr = FMW_GICR_BASE + GICR_ICFGR0;
        shift = (interrupt & GIC_INTS_PER_REG_MASK) << GIC_BITS_PER_INT_SHIFT;
        break;

    case INTERRUPT_TYPE_PPI:
        reg_addr = FMW_GICR_BASE + GICR_ICFGR1;
        shift = ((interrupt - GIC_PPI_BASE) & GIC_INTS_PER_REG_MASK)
            << GIC_BITS_PER_INT_SHIFT;
        break;

    case INTERRUPT_TYPE_SPI:
        n = interrupt >> GIC_INTS_PER_REG_SHIFT;
        shift = (interrupt & GIC_INTS_PER_REG_MASK) << GIC_BITS_PER_INT_SHIFT;
        reg_addr = FMW_GICD_BASE + GICD_ICFGR(n);
        break;

    default:
        return FWK_E_SUPPORT;
    }

    val = fwk_mmio_read_32(reg_addr);
    val &= ~(GIC_CFG_MASK << shift);
    val |= ((cfg & GIC_CFG_MASK) << shift);
    fwk_mmio_write_32(reg_addr, val);

    return FWK_SUCCESS;
}

int arch_interrupt_set_pending(unsigned int interrupt)
{
    switch (interrupt_type_from_id(interrupt)) {
    case INTERRUPT_TYPE_SGI:
    case INTERRUPT_TYPE_PPI:
        fwk_mmio_write_32(
            FMW_GICR_BASE + GICR_SGI_BASE + GICR_ISPENDR0, FWK_BIT(interrupt));
        break;
    case INTERRUPT_TYPE_SPI:
        fwk_mmio_write_32(
            FMW_GICD_BASE + GICD_ISPENDR(interrupt / 32),
            FWK_BIT(interrupt % 32));
        break;
    case INTERRUPT_TYPE_OTHER:
        return FWK_E_SUPPORT;
    }

    return FWK_SUCCESS;
}

int arch_interrupt_set_priority(unsigned int interrupt, unsigned int priority)
{
    switch (interrupt_type_from_id(interrupt)) {
    case INTERRUPT_TYPE_SPI:
        unsigned int reg = interrupt >> 2;
        unsigned offset = interrupt % 4;
        uint32_t shift = offset * 8;
        uint32_t mask = 0xFFu << shift;
        uintptr_t reg_addr = FMW_GICD_BASE + GICD_IPRIORITY + (reg << 2);

        uint32_t reg_val = fwk_mmio_read_32(reg_addr);
        reg_val = (reg_val & ~mask) | ((uint32_t)priority << shift);
        fwk_mmio_write_32(reg_addr, reg_val);
        break;
    default:
        return FWK_E_SUPPORT;
    }

    return FWK_SUCCESS;
}

int arch_interrupt_clear_pending(unsigned int interrupt)
{
    switch (interrupt_type_from_id(interrupt)) {
    case INTERRUPT_TYPE_SGI:
    case INTERRUPT_TYPE_PPI:
        fwk_mmio_write_32(
            FMW_GICR_BASE + GICR_SGI_BASE + GICR_ICPENDR0, FWK_BIT(interrupt));
        break;
    case INTERRUPT_TYPE_SPI:
        fwk_mmio_write_32(
            FMW_GICD_BASE + GICD_ICPENDR(interrupt / 32),
            FWK_BIT(interrupt % 32));
        break;
    case INTERRUPT_TYPE_OTHER:
        return FWK_E_SUPPORT;
    }

    return FWK_SUCCESS;
}

int arch_interrupt_set_isr_irq(unsigned int interrupt, void (*isr)(void))
{
    struct isr_callback *entry;

    if (interrupt >= INTERRUPT_ID_ISR_LIMIT || isr == NULL) {
        return FWK_E_PARAM;
    }

    entry = &callback[interrupt];
    entry->func = isr;
    entry->param = 0;
    entry->use_param = false;

    return FWK_SUCCESS;
}

int arch_interrupt_set_isr_irq_param(
    unsigned int interrupt,
    void (*isr)(uintptr_t param),
    uintptr_t parameter)
{
    struct isr_callback *entry;

    if (interrupt >= INTERRUPT_ID_ISR_LIMIT || isr == NULL) {
        return FWK_E_PARAM;
    }

    entry = &callback[interrupt];
    entry->func_with_param = isr;
    entry->param = parameter;
    entry->use_param = true;

    return FWK_SUCCESS;
}

int arch_interrupt_set_isr_nmi(void (*isr)(void))
{
    return FWK_E_SUPPORT;
}

int arch_interrupt_set_isr_nmi_param(
    void (*isr)(uintptr_t),
    uintptr_t parameter)
{
    return FWK_E_SUPPORT;
}

int arch_interrupt_set_isr_dummy(void (*isr)(void))
{
    return FWK_E_SUPPORT;
}

int arch_interrupt_set_isr_dummy_param(
    void (*isr)(uintptr_t param),
    uintptr_t parameter)
{
    return FWK_E_SUPPORT;
}

int arch_interrupt_get_current(unsigned int *interrupt)
{
    if (interrupt == NULL) {
        return FWK_E_PARAM;
    }

    if (current_iar >= INTERRUPT_ID_ISR_LIMIT) {
        return FWK_E_STATE;
    }

    *interrupt = current_iar;

    /* Not an interrupt */
    if (*interrupt == INTERRUPT_ID_INVALID) {
        return FWK_E_STATE;
    }

    return FWK_SUCCESS;
}

bool arch_interrupt_is_interrupt_context(void)
{
    return current_iar != INTERRUPT_ID_INVALID;
}

int arch_interrupt_init(void)
{
#ifdef GIC_V2
    unsigned int val;

    /*
     * Enable the Group 0 interrupts, FIQEn and disable Group 0/1
     * bypass.
     */
    val = CTLR_ENABLE_G0_BIT | FIQ_EN_BIT | FIQ_BYP_DIS_GRP0;
    val |= IRQ_BYP_DIS_GRP0 | FIQ_BYP_DIS_GRP1 | IRQ_BYP_DIS_GRP1;

    /* Program the idle priority in the PMR */
    fwk_mmio_write_32(FMW_GICD_BASE + GICC_BASE + GICC_PMR, GIC_PRI_MASK);
    fwk_mmio_write_32(FMW_GICD_BASE + GICC_BASE + GICC_CTLR, val);
#endif

    return FWK_SUCCESS;
}
