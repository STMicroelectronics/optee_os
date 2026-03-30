/*
 * Arm SCP/MCP Software
 * Copyright (c) 2025, Arm Limited and Contributors. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 * Description:
 *     Safety Island SSU register definitions
 */

/*!
 * \file
 * \brief SSU internal register definitions.
 */

#ifndef SI0_SSU_H
#define SI0_SSU_H

#include <fwk_macros.h>

#include <stdint.h>

/* Pointer to RoS Clock register block */
#define SSU_REG_PTR ((uintptr_t)&ssu_reg)

/* Error values to enable and disable Error control register */
#define ERROR_ENABLE  1
#define ERROR_DISABLE 0

/* Invalid FSM state to test negative scenario */
#define INVALID_FSM_STATE 9

/* SSU Register Map */
struct ssu_reg {
    /* 0x000: Error Feature Register (RO) */
    FWK_R uint32_t ERR_FR;

    uint8_t RESERVED0[0x008 - 0x004];

    /* 0x008: Error Control Register (RW) */
    FWK_RW uint32_t ERR_CTRL;

    uint8_t RESERVED1[0x010 - 0x00C];

    /* 0x010: Error Status Register (RW) */
    FWK_RW uint32_t ERR_STATUS;

    uint8_t RESERVED2[0x800 - 0x014];

    /* 0x800: Implementation-defined Error Control Register (RW) */
    FWK_RW uint32_t ERR_IMPDEF;

    /* 0x804: Access Key Register (RW) */
    FWK_RW uint32_t SYS_KEY;

    /* 0x808: SSU Status Register (RO) */
    FWK_R uint32_t SYS_STATUS;

    uint8_t RESERVED3[0x810 - 0x80C];

    /* 0x810: SSU FSM Control Register (RAZ - Read-As-Zero) */
    FWK_RW uint32_t SYS_CTRL;
};

struct ssu_reg ssu_reg;

#endif /* SI0_SSU_H */
