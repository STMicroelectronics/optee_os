/*
 * Arm SCP/MCP Software
 * Copyright (c) 2023-2024, Arm Limited and Contributors. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 * Description:
 *     Definitions for mock PSU module configuration.
 */

#ifndef TC4_MOCK_PSU_H
#define TC4_MOCK_PSU_H

enum mock_psu_id {
    MOCK_PSU_ELEMENT_IDX_GROUP_LITTLE,
    MOCK_PSU_ELEMENT_IDX_GROUP_MID,
    MOCK_PSU_ELEMENT_IDX_GROUP_BIG,
    MOCK_PSU_ELEMENT_IDX_GPU,
    MOCK_PSU_ELEMENT_IDX_COUNT,
};

#endif /* TC4_MOCK_PSU_H */
