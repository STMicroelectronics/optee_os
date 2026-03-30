/*
 * Arm SCP/MCP Software
 * Copyright (c) 2024, Arm Limited and Contributors. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 * Description:
 *     Base address definitions for the SCP's sub-system and access extending
 *     into the rest of the CSS.
 */

#ifndef SCP_CSS_MMAP_H
#define SCP_CSS_MMAP_H

// clang-format off

/* Base address and size of SCP's ITCM */
#define SCP_ITC_RAM_BASE (0x00000000)
#define SCP_ITC_RAM_SIZE (256 * 1024)

/* Base address and size of SCP's DTCM */
#define SCP_DTC_RAM_BASE (0x20000000)
#define SCP_DTC_RAM_SIZE (256 * 1024)

/* SCP sub-system peripherals */
#define SCP_REFCLK_CNTCONTROL_BASE (0x1A430000)
#define SCP_SID_BASE (0x1A4A0000)
#define SCP_REFCLK_CNTCTL_BASE (0x44000000)
#define SCP_REFCLK_CNTBASE0_BASE (0x44001000)
#define SCP_UART_BASE (0x44002000)
#define SCP_SCP2AP_MHUV3_SEND_S_BASE (0x45020000)
#define SCP_AP2SCP_MHUV3_RCV_S_BASE (0x45030000)
#define SCP_SCP2RSS_MHUV3_SEND_BASE (0x46000000)
#define SCP_RSS2SCP_MHUV3_RCV_BASE (0x46010000)
#define SCP_POWER_CONTROL_BASE (0x50000000)
#define SCP_ATU_BASE (0x50010000)
#define SCP_SYSTEM_PIK_BASE (0x50040000)
#define SCP_PPU_SYS0_BASE (0x50041000)

/* SCP addresses mapped via ATU into address translation windows */
#define SCP_ADDRESS_TRANSLATION_WINDOW0_BASE (0x60000000)
#define SCP_ADDRESS_TRANSLATION_WINDOW1_BASE (0xA0000000)

// clang-format on

#endif /* SCP_CSS_MMAP_H */
