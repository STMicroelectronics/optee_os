/*
 * Arm SCP/MCP Software
 * Copyright (c) 2024, Arm Limited and Contributors. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef FMW_MEMORY_H
#define FMW_MEMORY_H

#include "fvp_baser_aemv8r_mmap.h"

#include <fwk_macros.h>

#define FMW_MEM_MODE ARCH_MEM_MODE_DUAL_REGION_NO_RELOCATION

/*
 * Instruction RAM
 */
#define FMW_MEM0_BASE FVP_ITC_RAM_BASE
#define FMW_MEM0_SIZE FVP_ITC_RAM_SIZE

/*
 * Data RAM
 */
#define FMW_MEM1_BASE FVP_DTC_RAM_BASE
#define FMW_MEM1_SIZE FVP_DTC_RAM_SIZE

/*
 * Stack
 */
#define FMW_STACK_SIZE (2 * FWK_KIB)

#endif /* FMW_MEMORY_H */
