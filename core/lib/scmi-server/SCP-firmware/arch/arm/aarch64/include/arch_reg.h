/*
 * Arm SCP/MCP Software
 * Copyright (c) 2024-2025, Arm Limited and Contributors. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef ARCH_REG_H
#define ARCH_REG_H

#include <fwk_macros.h>

#define READ_SYSREG(reg) \
    ({ \
        uint64_t reg_value; \
        __asm__ volatile("mrs %0, " FWK_STRINGIFY(reg) \
                         : "=r"(reg_value)::"memory"); \
        reg_value; \
    })

#define WRITE_SYSREG(reg, value) \
    ({ \
        __asm__ volatile("msr " FWK_STRINGIFY(reg) ", %x0" ::"r"(value) \
                         : "memory"); \
    })

#define BARRIER_DSYNC_FENCE_FULL() \
    ({ __asm__ volatile("dsb sy" ::: "memory"); })

#define BARRIER_ISYNC_FENCE_FULL() ({ __asm__ volatile("isb" ::: "memory"); })

#define icc_iar_el1   S3_0_c12_c8_0
#define icc_eoir0_el1 S3_0_c12_c8_1
#define mpuir_el2     S3_4_c0_c0_4
#define prselr_el2    S3_4_c6_c2_1
#define prbar_el2     S3_4_c6_c8_0
#define prlar_el2     S3_4_c6_c8_1

/*
 * PRBAR values
 */
#define PRBAR_BASE(val)          ((val)&FWK_GEN_MASK_64(47, 6))
#define PRBAR_SH(val)            ((val) << 4)
#define PRBAR_SH_NON_SHAREABLE   0x0
#define PRBAR_SH_OUTER_SHAREABLE 0x2
#define PRBAR_SH_INNER_SHAREABLE 0x3
#define PRBAR_AP(val)            ((val) << 2)
#define PRBAR_AP_RW_EL2          0x0
#define PRBAR_AP_RO_EL2          0x2
#define PRBAR_XN(val)            (val)
#define PRBAR_XN_PERMITTED       0x0
#define PRBAR_XN_NOT_PERMITTED   0x2
#define PRBAR_VALUE(base, sh, ap, xn) \
    (PRBAR_BASE(base) | PRBAR_SH(sh) | PRBAR_AP(ap) | PRBAR_XN(xn))

/*
 * PRLAR values
 */
#define PRLAR_LIMIT(limit)    ((limit)&FWK_GEN_MASK_64(47, 6))
#define PRLAR_NS(val)         ((val) << 4)
#define PRLAR_NS_SECURE       0x0
#define PRLAR_NS_NON_SECURE   0x1
#define PRLAR_ATTR_INDEX(val) ((val) << 1)
#define PRLAR_EN(val)         (val)
#define PRLAR_EN_DISABLED     0x0
#define PRLAR_EN_ENABLED      0x1
#define PRLAR_VALUE(limit, ns, attr_index, en) \
    (PRLAR_LIMIT(limit) | PRLAR_NS(ns) | PRLAR_ATTR_INDEX(attr_index) | \
     PRLAR_EN(en))

/*
 * MAIR values
 */
#define MAIR_DEVICE_NGNRNE 0x0
#define MAIR_NORMAL_WB_NT  0xFF
#define MAIR_NORMAL_NC     0x44

/*
 * SCTLR_EL3 values
 */
#define SCTLR_EL3_RES1 \
    (FWK_BIT(29) | FWK_BIT(28) | FWK_BIT(23) | FWK_BIT(18) | FWK_BIT(16) | \
     FWK_BIT(5) | FWK_BIT(4))
#define SCTLR_EL3_RESET SCTLR_EL3_RES1
#define SCTLR_EL3_NAA   FWK_BIT(6)

/*
 * SCR_EL3 values
 */
#define SCR_EL3_RW FWK_BIT(10)

/*
 * SPSR_EL3 values
 */
#define SPSR_EL3_A      FWK_BIT(8)
#define SPSR_EL3_I      FWK_BIT(7)
#define SPSR_EL3_F      FWK_BIT(6)
#define SPSR_EL3_M_EL2H 0b1001 /* EL2 is in handler mode */

/*
 * SCTLR_EL2 values
 */
#define SCTLR_EL2_RES1 \
    (FWK_BIT(29) | FWK_BIT(28) | FWK_BIT(23) | FWK_BIT(22) | FWK_BIT(18) | \
     FWK_BIT(16) | FWK_BIT(11) | FWK_BIT(5) | FWK_BIT(4))
#define SCTLR_EL2_RESET SCTLR_EL2_RES1
#define SCTLR_EL2_I     FWK_BIT(12)
#define SCTLR_EL2_C     FWK_BIT(2)
#define SCTLR_EL2_M     FWK_BIT(0)

/*
 * HCR_EL2 values
 */
#define HCR_EL2_SWIO  FWK_BIT(1)
#define HCR_EL2_FMO   FWK_BIT(3)
#define HCR_EL2_IMO   FWK_BIT(4)
#define HCR_EL2_AMO   FWK_BIT(5)
#define HCR_EL2_RESET (HCR_EL2_SWIO | HCR_EL2_FMO | HCR_EL2_IMO | HCR_EL2_AMO)

/*
 * ID_AA64MMFR0_EL1 values
 */
#define ID_AA64MMFR0_EL1_MSA_MASK      FWK_GEN_MASK_64(51, 48)
#define ID_AA64MMFR0_EL1_MSA_FRAC_MASK FWK_GEN_MASK_64(55, 52)

/*
 * DAIF bits
 */
#define DAIF_FIQ FWK_BIT(0)

/*
 * GIC registers
 */
#define GICR_SGI_BASE 0x10000u

#define GICR_ISENABLER0 0x0100u
#define GICR_ICENABLER0 0x0180u
#define GICR_ISPENDR0   0x0200u
#define GICR_ICPENDR0   0x0280u
#define GICD_IPRIORITY  0x0400u

#define GICD_ISENABLER(N) (0x0100u + (4 * (N)))
#define GICD_ICENABLER(N) (0x0180u + (4 * (N)))
#define GICD_ISPENDR(N)   (0x0200u + (4 * (N)))
#define GICD_ICPENDR(N)   (0x0280u + (4 * (N)))
#define GICD_ICFGR(N)     (0x0C00u + (4 * (N)))

#define GICR_ICFGR0    0x0C00u
#define GICR_ICFGR1    0x0C04u
#define GICR_IGRPMODR0 0x0D00u

#define GIC_LEVEL_TRIGGER_INTR 0x0
#define GIC_EDGE_TRIGGER_INTR  0x2

/*
 * GIC V2 registers
 */
#define GICC_BASE 0x10000u

#define FIQ_EN_BIT         FWK_BIT(3)
#define CTLR_ENABLE_G0_BIT FWK_BIT(0)

#define FIQ_BYP_DIS_GRP0 FWK_BIT(5)
#define IRQ_BYP_DIS_GRP0 FWK_BIT(6)
#define FIQ_BYP_DIS_GRP1 FWK_BIT(7)
#define IRQ_BYP_DIS_GRP1 FWK_BIT(8)

#define GICC_CTLR    (0x0)
#define GICC_PMR     (0x4)
#define GIC_PRI_MASK (0xff)
#define GICC_IAR     (0xC)
#define GICC_EOIR    (0x10)

#endif /* ARCH_REG_H */
