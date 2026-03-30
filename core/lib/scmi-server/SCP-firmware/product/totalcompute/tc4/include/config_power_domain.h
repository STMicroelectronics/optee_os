/*
 * Arm SCP/MCP Software
 * Copyright (c) 2023-2024, Arm Limited and Contributors. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef CONFIG_POWER_DOMAIN_H
#define CONFIG_POWER_DOMAIN_H

#include <tc4_core.h>

#include <stdint.h>

#define PD_STATIC_CME_INIT(_cme) PD_STATIC_DEV_IDX_CME##_cme

/*
 * Power domain indices for the statically defined domains used for:
 * - Indexing the domains in the tc4_power_domain_static_element_table
 * - Indexing the SYSTOP children in the power domain tree
 *
 * When calculating a power domain element index, use the formula:
 * core_count + cluster_count + pd_static_dev_idx
 */
enum pd_static_dev_idx {
    TC4_FOR_EACH_CME(PD_STATIC_CME_INIT),
    PD_STATIC_DEV_IDX_GPUTOP,
    PD_STATIC_DEV_IDX_SYSTOP,
    PD_STATIC_DEV_IDX_COUNT,
    PD_STATIC_DEV_IDX_NONE = UINT32_MAX
};

#endif /* CONFIG_POWER_DOMAIN_H */
