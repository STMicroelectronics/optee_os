/*
 * Arm SCP/MCP Software
 * Copyright (c) 2024-2025, Arm Limited and Contributors. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef INTERNAL_GICX00_REG_H
#define INTERNAL_GICX00_REG_H

#include <stdint.h>

#define GICD_CTLR  0x0000u
#define GICD_TYPER 0x0004u

#define GICR_WAKER 0x0014u
#define GICR_IIDR  0x0004u
#define GICR_PWRR  0x0024u

#define GICD_TYPER_IT_LINES_NUMBER 0x1Fu

#define GICD_CTLR_ENABLE_GROUP_0 FWK_BIT(0)
#define GICD_CTLR_ARE_S          FWK_BIT(4)
#define GICD_CTLR_DS             FWK_BIT(6)

#define GICR_IIDR_PRODUCT_ID     FWK_GEN_MASK(31, 24)
#define GICR_IIDR_IMPLEMENTER_ID FWK_GEN_MASK(11, 0)
#define GICR_IIDR_GIC600         0x0200043Bu
#define GICR_IIDR_GIC600AE       0x0300043Bu
#define GICR_IIDR_GIC700         0x0400043Bu
#define GICR_IIDR_GIC720AE       0x0700043Bu

#define GICR_WAKER_PROCESSOR_SLEEP FWK_BIT(1)
#define GICR_WAKER_CHILDREN_ASLEEP FWK_BIT(2)

#define GICR_PWRR_RDPD FWK_BIT(0)

#define GICR_SGI_BASE 0x10000u

#define GICR_IGROUPR0      0x0080u
#define GICR_ISENABLER0    0x0100u
#define GICR_IPRIORITYR(N) (0x0400u + (4 * (N)))
#define GICR_ICFGR1        0x0C04u
#define GICR_IGRPMODR0     0x0D00u

#define GICD_IGROUPR(N)    (0x0080u + (4 * (N)))
#define GICD_ISENABLER(N)  (0x0100u + (4 * (N)))
#define GICD_IPRIORITYR(N) (0x0400u + (4 * (N)))
#define GICD_ICFGR(N)      (0x0C00u + (4 * (N)))
#define GICD_IGRPMODR(N)   (0x0D00u + (4 * (N)))
#define GICD_IROUTER(N)    (0x6000u + (8 * (N)))

#define ICC_SRE_SRE    FWK_BIT(0)
#define ICC_SRE_DFB    FWK_BIT(1)
#define ICC_SRE_DIB    FWK_BIT(2)
#define ICC_SRE_ENABLE FWK_BIT(3)

#define ICC_IGRPEN0_ENABLE FWK_BIT(0)

#define ICC_PMR_MAX 0xFFu

#define MPDIR_AFFINITY_MASK 0xFF00FFFFFFull

#define INTERRUPT_ID_PPI_LIMIT 32u
#define INTERRUPT_ID_LIMIT     1020u

void write_icc_sre(uint64_t value);
void write_igrpen0_el1(uint64_t value);
void write_icc_pmr(uint64_t value);
uint64_t read_mpidr_el1(void);

#endif /* INTERNAL_GICX00_REG_H */
