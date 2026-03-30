/*
 * Arm SCP/MCP Software
 * Copyright (c) 2024-2025, Arm Limited and Contributors. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include "internal/gicx00_reg.h"

#ifndef BUILD_TESTS
#    include <arch_reg.h>
#endif

#include <mod_gicx00.h>

#include <fwk_assert.h>
#include <fwk_id.h>
#include <fwk_interrupt.h>
#include <fwk_mmio.h>
#include <fwk_module.h>
#include <fwk_status.h>

#include <stddef.h>

static void gic600_init(const struct mod_gicx00_config *config)
{
    uint32_t reg;

    /* Power on redistributor */
    fwk_mmio_write_32(config->gicr_base + GICR_PWRR, 0);
    do {
        reg = fwk_mmio_read_32(config->gicr_base + GICR_PWRR);
    } while ((reg & GICR_PWRR_RDPD) != 0);
}

static void initialize_interrupts(const struct mod_gicx00_config *config)
{
    uint32_t num_irqs;
    uint32_t index;
    uint64_t mpidr;

    num_irqs = fwk_mmio_read_32(config->gicd_base + GICD_TYPER) &
        GICD_TYPER_IT_LINES_NUMBER;
    /* The formula for the maximum SPI interrupt ID is taken from
       section 12.9.38 of the GIC specification */
    num_irqs = 32 * (num_irqs + 1);
    fwk_assert(num_irqs >= INTERRUPT_ID_PPI_LIMIT);
    fwk_assert(num_irqs < INTERRUPT_ID_LIMIT);

    /* Initialize SGI/PPI registers */
    fwk_mmio_write_32(config->gicr_base + GICR_SGI_BASE + GICR_IGROUPR0, 0);
    fwk_mmio_write_32(config->gicr_base + GICR_SGI_BASE + GICR_ISENABLER0, 0);
    fwk_mmio_write_32(config->gicr_base + GICR_SGI_BASE + GICR_ICFGR1, 0);
    fwk_mmio_write_32(config->gicr_base + GICR_SGI_BASE + GICR_IGRPMODR0, 0);
    for (index = 0u; index < INTERRUPT_ID_PPI_LIMIT / 4; index++) {
        fwk_mmio_write_32(
            config->gicr_base + GICR_SGI_BASE + GICR_IPRIORITYR(index), 0);
    }

    /* Initialize SPI registers */
    for (index = INTERRUPT_ID_PPI_LIMIT / 32; index < num_irqs / 32; index++) {
        fwk_mmio_write_32(config->gicd_base + GICD_IGROUPR(index), 0);
        fwk_mmio_write_32(config->gicd_base + GICD_ISENABLER(index), 0);
        fwk_mmio_write_32(config->gicd_base + GICD_IGRPMODR(index), 0);
    }
    for (index = INTERRUPT_ID_PPI_LIMIT / 16; index < num_irqs / 16; index++) {
        fwk_mmio_write_32(config->gicd_base + GICD_ICFGR(index), 0);
    }
    for (index = INTERRUPT_ID_PPI_LIMIT / 4; index < num_irqs / 4; index++) {
        fwk_mmio_write_32(config->gicd_base + GICD_IPRIORITYR(index), 0);
    }

    /* Route all interrupts to the current processing element */
    mpidr = read_mpidr_el1() & MPDIR_AFFINITY_MASK;
    for (index = INTERRUPT_ID_PPI_LIMIT; index < num_irqs; index++) {
        fwk_mmio_write_64(config->gicd_base + GICD_IROUTER(index), mpidr);
    }
}

static int gicx00_init(
    fwk_id_t module_id,
    unsigned int element_count,
    const void *data)
{
    const struct mod_gicx00_config *config;
    uint32_t reg;

    fwk_assert(element_count == 0);
    fwk_assert(data != NULL);

    config = data;

    /* Initialize GICD_CTLR */
    fwk_mmio_write_32(
        config->gicd_base + GICD_CTLR,
        GICD_CTLR_ENABLE_GROUP_0 | GICD_CTLR_ARE_S | GICD_CTLR_DS);

    /* Perform GIC-specific initialization */
    reg = fwk_mmio_read_32(config->gicr_base + GICR_IIDR) &
        (GICR_IIDR_PRODUCT_ID | GICR_IIDR_IMPLEMENTER_ID);
    switch (reg) {
    case GICR_IIDR_GIC600:
    case GICR_IIDR_GIC600AE:
    case GICR_IIDR_GIC700:
    case GICR_IIDR_GIC720AE:
        gic600_init(config);
        break;
    default:
        break;
    }

    /* Wake up redistributor */
    reg = fwk_mmio_read_32(config->gicr_base + GICR_WAKER);
    reg &= ~GICR_WAKER_PROCESSOR_SLEEP;
    fwk_mmio_write_32(config->gicr_base + GICR_WAKER, reg);
    do {
        reg = fwk_mmio_read_32(config->gicr_base + GICR_WAKER);
    } while (reg & GICR_WAKER_CHILDREN_ASLEEP);

    /* Initialize interrupt registers */
    initialize_interrupts(config);

    /* Enable CPU interface */
    write_icc_sre(ICC_SRE_ENABLE | ICC_SRE_DIB | ICC_SRE_DFB | ICC_SRE_SRE);
    write_icc_pmr(ICC_PMR_MAX);
    write_igrpen0_el1(ICC_IGRPEN0_ENABLE);
#ifndef BUILD_TESTS
    BARRIER_DSYNC_FENCE_FULL();
    BARRIER_ISYNC_FENCE_FULL();
#endif

    /* Unmask interrupts */
    fwk_interrupt_global_enable(0);

    return FWK_SUCCESS;
}

/* Module description */
const struct fwk_module module_gicx00 = {
    .type = FWK_MODULE_TYPE_DRIVER,
    .init = gicx00_init,
};
