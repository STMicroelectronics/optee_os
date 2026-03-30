/*
 * Arm SCP/MCP Software
 * Copyright (c) 2024, Arm Limited and Contributors. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 */

#ifndef SYSTEM_CONTROL_REGS_SSE310_H
#define SYSTEM_CONTROL_REGS_SSE310_H

#include <fwk_macros.h>

#include <stdint.h>

/*!
 * \brief SCP (SSE-310) System Control Block register definitions
 */
struct scp_sysctrl_reg {
    FWK_R uint32_t SECDBGSTAT; /* 0x000 (R/ ) Secure Debug
                                *             Configuration Status */
    FWK_RW uint32_t SECDBGSET; /* 0x004 (R/W) Secure Debug
                                *             Configuration Set */
    FWK_W uint32_t SECDBGCLR; /* 0x008 ( /W) Secure Debug
                               *             Configuration Clear */
    FWK_RW uint32_t SCSECCTRL; /* 0x00C (R/W) System Control Security
                                *             Controls */
    FWK_RW uint32_t CLK_CFG0; /* 0x010 (R/W) Clock Configuration 0 */
    FWK_RW uint32_t CLK_CFG1; /* 0x014 (R/W) Clock Configuration 1 */
    FWK_RW uint32_t CLOCK_FORCE; /* 0x018 (R/W) Clock Forces */
    FWK_RW uint32_t CLK_CFG2; /* 0x01C (R/W) Clock Configuration 2 */
    FWK_R uint32_t RESERVED0[56];
    FWK_RW uint32_t RESET_SYNDROME; /* 0x100 (R/W) Reset syndrome */
    FWK_RW uint32_t RESET_MASK; /* 0x104 (R/W) Reset mask */
    FWK_W uint32_t SWRESET; /* 0x108 ( /W) Software Reset */
    FWK_RW uint32_t GRETREG; /* 0x10C (R/W) General Purpose
                              *             Retention */
    FWK_RW uint32_t INITSVTOR0; /* 0x110 (R/W) CPU 0 Initial Secure
                                 *             Reset Vector Register */
    FWK_R uint32_t RESERVED1[3];
    FWK_RW uint32_t CPUWAIT; /* 0x120 (R/W) CPU Boot Wait Control */
    FWK_RW uint32_t NMI_ENABLE; /* 0x124 (R/W) Non Maskable Interrupts
                                 *             Enable */
    FWK_RW uint32_t PPUINTSTAT; /* 0x128 (R/W) PPU Interrupt Status */
    FWK_R uint32_t RESERVED2[52];
    FWK_RW uint32_t PWRCTRL; /* 0x1FC (R/W) Power Configuration and
                              *             Control */
    FWK_RW uint32_t PDCM_PD_SYS_SENSE; /* 0x200 (R/W) PDCM PD_SYS
                                        *             Sensitivity */
    FWK_R uint32_t PDCM_PD_CPU0_SENSE; /* 0x204 (R/ ) PDCM PD_CPU0
                                        *             Sensitivity */
    FWK_R uint32_t RESERVED3[3];
    FWK_RW uint32_t PDCM_PD_VMR0_SENSE; /* 0x214 (R/W) PDCM PD_VMR0
                                         *             Sensitivity */
    FWK_RW uint32_t PDCM_PD_VMR1_SENSE; /* 0x218 (R/W) PDCM PD_VMR1
                                         *             Sensitivity */
    FWK_R uint32_t RESERVED4[12];
    FWK_RW uint32_t PDCM_PD_MGMT_SENSE; /* 0x24C (R/W) PDCM PD_MGMT
                                         *             Sensitivity */
    FWK_RW uint32_t GRETEXREG; /* 0x250 (R/W) General Purpose
                                *             Retention Expansion */
    FWK_RW uint32_t DMA_BOOT_EN; /* 0x254 (R/W) DMA-350 Boot Enable
                                  *             Static Config */
    FWK_RW uint32_t DMA_BOOT_ADDR; /* 0x258 (R/W) DMA-350 Boot Address
                                    *             Static Config */
    FWK_RW uint32_t LCM_DCU_FORCE_DIS; /* 0x25C (R/W) LCM DCU Force
                                        *             Disable */
    FWK_R uint32_t RESERVED5[860];
    FWK_R uint32_t PIDR4; /* 0xFD0 (R/ ) Peripheral ID 4 */
    FWK_R uint32_t RESERVED6[3];
    FWK_R uint32_t PIDR0; /* 0xFE0 (R/ ) Peripheral ID 0 */
    FWK_R uint32_t PIDR1; /* 0xFE4 (R/ ) Peripheral ID 1 */
    FWK_R uint32_t PIDR2; /* 0xFE8 (R/ ) Peripheral ID 2 */
    FWK_R uint32_t PIDR3; /* 0xFEC (R/ ) Peripheral ID 3 */
    FWK_R uint32_t CIDR0; /* 0xFF0 (R/ ) Component ID 0 */
    FWK_R uint32_t CIDR1; /* 0xFF4 (R/ ) Component ID 1 */
    FWK_R uint32_t CIDR2; /* 0xFF8 (R/ ) Component ID 2 */
    FWK_R uint32_t CIDR3; /* 0xFFC (R/ ) Component ID 3 */
};

/* SWRESET bit definitions */
#define SCP_SCB_SWRESET_POS 9

#endif /* SYSTEM_CONTROL_REGS_SSE310_H */
