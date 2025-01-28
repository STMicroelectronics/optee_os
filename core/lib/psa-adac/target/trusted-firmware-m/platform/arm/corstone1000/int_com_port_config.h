/*
 * Copyright (c) 2021, Arm Limited. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 */

#ifndef _HAL_H
#define _HAL_H

#ifdef __cplusplus
extern "C"
{
#endif

#define HAL_SOCID_SIZE (32)

#define SE_HOST_ACCESS  0x60000000
#define SE_APBCOM_BASE  (SE_HOST_ACCESS + 0x1B900000)

#define HAL_APBCOM_BASE SE_APBCOM_BASE

#ifdef __cplusplus
}
#endif

#endif /* _HAL_H */



