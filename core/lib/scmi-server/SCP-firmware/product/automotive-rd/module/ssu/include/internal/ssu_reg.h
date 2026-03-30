/*
 * Arm SCP/MCP Software
 * Copyright (c) 2025, Arm Limited and Contributors. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 * Description:
 *      SSU register definitions
 */

/*!
 * \file
 * \brief SSU register definitions and access utilities.
 */

#ifndef SSU_REG_H
#define SSU_REG_H

#include <fwk_assert.h>
#include <fwk_macros.h>
#include <fwk_mmio.h>

#include <stdint.h>

/* Error Feature register */
#define SSU_ERR_FR 0x0000
/* Error control register */
#define SSU_ERR_CTRL 0x0008
/* Error status register */
#define SSU_ERR_STATUS 0x0010
/* Error Detect Enable register */
#define SSU_ERR_IMPDEF 0x0800

/* Sys Key register */
#define SSU_SYS_KEY 0x0804
/* Sys Status register */
#define SSU_SYS_STATUS 0x0808
/* Sys Control register */
#define SSU_SYS_CTRL 0x0810
/* Sys SW implementation signal register */
#define SSU_STATUS_DETAIL 0x0814

/* Register Field Definitions */

#define SSU_ERR_FR_ED_MASK  0x0003
#define SSU_ERR_FR_ED_SHIFT 0

#define SSU_ERR_CTRL_ED_MASK  0x0001
#define SSU_ERR_CTRL_ED_SHIFT 0

#define SSU_ERR_STATUS_SERR_MASK             0x00FF
#define SSU_ERR_STATUS_SERR_SHIFT            0
#define SSU_ERR_STATUS_IERR_MASK             0x1F00
#define SSU_ERR_STATUS_IERR_SHIFT            8
#define SSU_ERR_STATUS_IERR_APB_SW_MASK      0x0100
#define SSU_ERR_STATUS_IERR_APB_SW_SHIFT     8
#define SSU_ERR_STATUS_IERR_INC_SEQ_MASK     0x0200
#define SSU_ERR_STATUS_IERR_INC_SEQ_SHIFT    9
#define SSU_ERR_STATUS_IERR_APB_PARITY_MASK  0x0400
#define SSU_ERR_STATUS_IERR_APB_PARITY_SHIFT 10
#define SSU_ERR_STATUS_IERR_ERR_IN_MASK      0x1000
#define SSU_ERR_STATUS_IERR_ERR_IN_SHIFT     12
#define SSU_ERR_STATUS_OF_MASK               0x08000000
#define SSU_ERR_STATUS_OF_SHIFT              27
#define SSU_ERR_STATUS_VALID_MASK            0x40000000
#define SSU_ERR_STATUS_VALID_SHIFT           30

#define SSU_ERR_IMPDEF_CR_EN_MASK        0x0001
#define SSU_ERR_IMPDEF_CR_EN_SHIFT       0
#define SSU_ERR_IMPDEF_NCR_EN_MASK       0x0002
#define SSU_ERR_IMPDEF_NCR_EN_SHIFT      1
#define SSU_ERR_IMPDEF_APB_SW_EN_MASK    0x0010
#define SSU_ERR_IMPDEF_APB_SW_EN_SHIFT   4
#define SSU_ERR_IMPDEF_INC_SEQ_EN_MASK   0x0020
#define SSU_ERR_IMPDEF_INC_SEQ_EN_SHIFT  5
#define SSU_ERR_IMPDEF_APB_PROT_EN_MASK  0x0040
#define SSU_ERR_IMPDEF_APB_PROT_EN_SHIFT 6

#define SSU_SYS_KEY_VALUE 0x000000BE

#define SSU_SYS_CTRL_MASK  0x003
#define SSU_SYS_CTRL_SHIFT 0

static inline uint32_t ssu_reg_read32(uintptr_t reg_base, uint32_t offset)
{
    return fwk_mmio_read_32(reg_base + offset);
}

static inline void ssu_reg_write32(
    uintptr_t reg_base,
    uint32_t offset,
    uint32_t value)
{
    fwk_mmio_write_32((reg_base + SSU_SYS_KEY), SSU_SYS_KEY_VALUE);
    fwk_mmio_write_32(reg_base + offset, value);
}

static inline void ssu_set_mask(
    uintptr_t reg_base,
    uint32_t offset,
    uint32_t mask)
{
    fwk_mmio_write_32((reg_base + SSU_SYS_KEY), SSU_SYS_KEY_VALUE);
    fwk_mmio_setbits_32(reg_base + offset, mask);
}

static inline void ssu_clear_mask(
    uintptr_t reg_base,
    uint32_t offset,
    uint32_t mask)
{
    fwk_mmio_write_32((reg_base + SSU_SYS_KEY), SSU_SYS_KEY_VALUE);
    fwk_mmio_clrbits_32(reg_base + offset, mask);
}

static inline void ssu_clear_set_mask(
    uintptr_t reg_base,
    uint32_t offset,
    uint32_t mask,
    uint32_t shift,
    uint32_t value)
{
    uint32_t reg;

    fwk_assert(shift <= 31);

    reg =
        (((ssu_reg_read32(reg_base, offset)) & ~(mask)) |
         ((value << shift) & (mask)));
    ssu_reg_write32(reg_base, offset, reg);
}
#endif /* SSU_REG_H */
