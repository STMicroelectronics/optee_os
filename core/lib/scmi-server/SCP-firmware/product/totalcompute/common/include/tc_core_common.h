/*
 * Arm SCP/MCP Software
 * Copyright (c) 2022-2024, Arm Limited and Contributors. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef TC_CORE_COMMON_H
#define TC_CORE_COMMON_H

#define TC_CORES_PER_CLUSTER  (8)
#define TC_NUMBER_OF_CLUSTERS (1)
#define TC_NUMBER_OF_CORES    (TC_CORES_PER_CLUSTER * TC_NUMBER_OF_CLUSTERS)

#define TC_FOR_EACH_CORE(_func) \
    _func(0), _func(1), _func(2), _func(3), _func(4), _func(5), _func(6), \
        _func(7)

#define TC_FOR_EACH_CLUSTER(_func) _func(0)

#endif /* TC_CORE_COMMON_H */
