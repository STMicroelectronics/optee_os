/*
 * Copyright (c) 2019, Arm Limited. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 */

#ifndef _INT_COM_PORT_HAL_API_H
#define _INT_COM_PORT_HAL_API_H

#ifdef __cplusplus
extern "C"
{
#endif

/*! @file
@brief This file contains the Hardware Abstraction API definition.
 */

/*
 * Include platform-specific hardware abstraction layer implementation header
 */
#include "int_com_port_config.h"

/* Mandatory implementation definitions */
#ifndef HAL_SOCID_SIZE
#error "HAL_SOCID_SIZE needs to be defined by the platform's hal.h"
#endif

#ifndef HAL_APBCOM_BASE
#error "HAL_APBCOM_BASE needs to be defined by the platform's hal.h"
#endif /* HAL_APBCOM_BASE */

/* Optional implementation definitions */
#ifndef HAL_OK
#define HAL_OK 0
#endif

#ifndef HAL_FAIL
#define HAL_FAIL 1UL
#endif

typedef uint32_t hal_socid_t[HAL_SOCID_SIZE / 4];

typedef uint32_t hal_status_t;

#ifdef __cplusplus
}
#endif

#endif /* _INT_COM_PORT_HAL_API_H */



