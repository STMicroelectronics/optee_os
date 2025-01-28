/*
 * Copyright (c) 2022, Arm Limited. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 */

#ifndef __INT_COM_PORT_CONFIG_H__
#define __INT_COM_PORT_CONFIG_H__

#include "platform_base_address.h"

#ifdef __cplusplus
extern "C" {
#endif

#define HAL_SOCID_SIZE (32)

/* FIXME - Check internal APB com address to see if secure or non-secure */
#define SE_HOST_ACCESS  HOST_ACCESS_BASE_S
#define SE_APBCOM_BASE  ((unsigned int)SE_HOST_ACCESS + 0x1B900000)

#define HAL_APBCOM_BASE SE_APBCOM_BASE

#ifdef __cplusplus
}
#endif

#endif /* __INT_COM_PORT_CONFIG_H__ */
