/* SPDX-License-Identifier: BSD-3-Clause */
/*
 * Copyright (c) 2025, STMicroelectronics
 */

#ifndef __STM32MP2_TAMP_H
#define __STM32MP2_TAMP_H

#include <tee_api_types.h>

TEE_Result stm32_activate_internal_tamper(int id);

#endif /* __STM32MP2_TAMP_H */
