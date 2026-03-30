/*
 * Arm SCP/MCP Software
 * Copyright (c) 2024-2025, Arm Limited and Contributors. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 * Description:
 *     Base address definitions for the MCP's sub-system and access extending
 *     into the rest of the CSS.
 */

#ifndef MCP_CSS_MMAP_H
#define MCP_CSS_MMAP_H

/* Base address and size of MCP's ITCM */
#define MCP_ITC_RAM_BASE (0x00000000)
#define MCP_ITC_RAM_SIZE (256 * 1024)

/* Base address and size of MCP's DTCM */
#define MCP_DTC_RAM_BASE (0x20000000)
#define MCP_DTC_RAM_SIZE (256 * 1024)

/* SCP sub-system peripherals */
#define MCP_MCP2SCP_MHUV3_SEND_S_BASE UINT32_C(0x45600000)
#define MCP_SCP2MCP_MHUV3_RCV_S_BASE  UINT32_C(0x45610000)
#define MCP_REFCLK_CNTCTL_BASE        (0x4C000000)
#define MCP_REFCLK_CNTBASE0_BASE      (0x4C001000)
#define MCP_UART_BASE                 (0x4C002000)
#define MCP_ATU_BASE                  UINT32_C(0x50010000)

/* MCP addresses translation window base addresses */
#define MCP_ADDRESS_TRANSLATION_WINDOW0_BASE (0x60000000)
#define MCP_ADDRESS_TRANSLATION_WINDOW1_BASE (0xA0000000)

/* Offsets within MCP's Address Translation Window0 */
#define MCP_ATW1_REFCLK_CNTCONTROL_OFFSET (0x2A430000)

/* MCP addresses mapped via ATU into address translation windows */
#define MCP_REFCLK_CNTCONTROL_BASE \
    (MCP_ADDRESS_TRANSLATION_WINDOW1_BASE + MCP_ATW1_REFCLK_CNTCONTROL_OFFSET)

/*
 * RSM memory region (shared between RSS, MCP, SCP) is mapped by ATU in
 * the MCP address translation window 0.
 */
#define MCP_SHARED_SRAM_RSM_BASE (MCP_ADDRESS_TRANSLATION_WINDOW0_BASE)

#endif /* MCP_CSS_MMAP_H */
