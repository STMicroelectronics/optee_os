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

#define CORES_PER_CLUSTER  4
#define NUMBER_OF_CLUSTERS 4

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
