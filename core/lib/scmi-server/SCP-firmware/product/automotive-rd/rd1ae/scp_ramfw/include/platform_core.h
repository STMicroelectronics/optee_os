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

#define NUMBER_OF_ROOTPORTS 5

#define CORES_PER_CLUSTER  1
#define NUMBER_OF_CLUSTERS 16

#define NUM_PCIE_INTEG_CTRL 4

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
