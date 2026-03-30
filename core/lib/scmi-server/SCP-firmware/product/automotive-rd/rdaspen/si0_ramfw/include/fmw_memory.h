/*
 * Arm SCP/MCP Software
 * Copyright (c) 2025, Arm Limited and Contributors. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef FMW_MEMORY_H
#define FMW_MEMORY_H

#include "si0_mmap.h"

#include <fwk_macros.h>

#define FMW_MEM_MODE ARCH_MEM_MODE_DUAL_REGION_NO_RELOCATION

/*
 * Instruction RAM
 */
#define FMW_MEM0_BASE SI0_ITC_RAM_BASE
#define FMW_MEM0_SIZE SI0_ITC_RAM_SIZE

/*
 * Data RAM
 */
#define FMW_MEM1_BASE SI0_DTC_RAM_BASE
#define FMW_MEM1_SIZE SI0_DTC_RAM_SIZE

/*
 * Stack
 */
#define FMW_STACK_SIZE (4 * FWK_KIB)

#endif /* FMW_MEMORY_H */
