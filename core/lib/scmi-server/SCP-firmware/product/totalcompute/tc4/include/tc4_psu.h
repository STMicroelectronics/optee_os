/*
 * Arm SCP/MCP Software
 * Copyright (c) 2023-2024, Arm Limited and Contributors. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 * Description:
 *     Definitions for PSU module configuration.
 */

#ifndef TC4_PSU_H
#define TC4_PSU_H

enum psu_id {
    PSU_ELEMENT_IDX_GROUP_LITTLE,
    PSU_ELEMENT_IDX_GROUP_MID,
    PSU_ELEMENT_IDX_GROUP_BIG,
    PSU_ELEMENT_IDX_GPU,
    PSU_ELEMENT_IDX_COUNT,
};

#endif /* TC4_PSU_H */
