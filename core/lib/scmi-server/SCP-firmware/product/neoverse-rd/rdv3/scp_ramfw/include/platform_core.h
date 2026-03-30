/*
 * Arm SCP/MCP Software
 * Copyright (c) 2025, Arm Limited and Contributors. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 * Description:
 *     Platform generic definitions.
 */

#ifndef PLATFORM_CORE_H
#define PLATFORM_CORE_H

#include <fwk_assert.h>

// clang-format off

#define PLATFORM_CORE_PER_CLUSTER_MAX 1

#define CORES_PER_CLUSTER  1
#if (PLATFORM_VARIANT == 0)
#define NUMBER_OF_CLUSTERS 16
#elif (PLATFORM_VARIANT == 1)
#define NUMBER_OF_CLUSTERS 8
#elif (PLATFORM_VARIANT == 2)
#    define NUMBER_OF_CLUSTERS 4
#else
#error "Unsupported PLATFORM_VARIANT value"
#endif

/* Peripheral address space per chip */
#define RDV3_CHIP_ADDR_SPACE  (64ULL * FWK_GIB)

/* SID controller register offsets, bit field masks and shifts */
#define SID_CHIP_ID_OFFSET  (0x60)
#define SID_CHIP_ID_CHIP_ID_MASK  (0x3F)
#define SID_CHIP_ID_CHIP_ID_SHIFT  (0x0)

#define SID_CHIP_ID_REG  (SCP_SID_BASE + SID_CHIP_ID_OFFSET)

// clang-format on

/* Number of chips supported on the platform. */
enum platform_chip_id {
    PLATFORM_CHIP_0,
#if (PLATFORM_VARIANT == 2)
    PLATFORM_CHIP_1,
    PLATFORM_CHIP_2,
    PLATFORM_CHIP_3,
#endif
    PLATFORM_CHIP_COUNT
};

static inline unsigned int platform_get_cluster_count(void)
{
    return NUMBER_OF_CLUSTERS;
}

static inline unsigned int platform_get_core_per_cluster_count(
    unsigned int cluster)
{
    fwk_assert(cluster < platform_get_cluster_count());

    return CORES_PER_CLUSTER;
}

static inline unsigned int platform_get_core_count(void)
{
    return platform_get_core_per_cluster_count(0) *
        platform_get_cluster_count();
}

#endif /* PLATFORM_CORE_H */
