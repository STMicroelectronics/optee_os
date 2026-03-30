/*
 * Arm SCP/MCP Software
 * Copyright (c) 2024, Arm Limited and Contributors. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 * Description:
 *     Base address definitions for the carveouts in SCP's Address Translation
 *     Window 1 (ATW1).
 */

#ifndef SCP_ATW1_MMAP_H
#define SCP_ATW1_MMAP_H

#include "scp_css_mmap.h"

/*
 * Offsets within SCP's Address Translation Window 1.
 *          __________________________
 *         |                          |
 *         |         CMN 1G           |
 *         |__________________________| 0xA0000000
 */
#define SCP_ATW1_CMN_BASE (SCP_ADDRESS_TRANSLATION_WINDOW1_BASE)
#define SCP_ATW1_CMN_SIZE (1 * FWK_GIB)

/* CMN config space is mapped in the SCP address translation window 1 */
#define SCP_CMN_BASE SCP_ATW1_CMN_BASE

#endif /* SCP_ATW1_MMAP_H */
