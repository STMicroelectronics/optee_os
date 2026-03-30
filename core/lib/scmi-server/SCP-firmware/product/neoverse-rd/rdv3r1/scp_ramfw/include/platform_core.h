/*
 * Arm SCP/MCP Software
 * Copyright (c) 2024, Arm Limited and Contributors. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 * Description:
 *     Platform generic definitions.
 */

#ifndef PLATFORM_CORE_H
#define PLATFORM_CORE_H

#include <fwk_assert.h>

/* Actual number of core and clusters implemented */
#define CORES_PER_CLUSTER  1
#define NUMBER_OF_CLUSTERS 14

/* Maximum number of clusters supported */
#define MAX_NUM_CLUSTERS 70

/* Maximum number of LCP sub-system instances supported */
#define MAX_NUM_LCP 7

/* Number of chips supported on the platform. */
enum platform_chip_id { PLATFORM_CHIP_0, PLATFORM_CHIP_1, PLATFORM_CHIP_COUNT };

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
