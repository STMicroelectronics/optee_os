/* SPDX-License-Identifier: BSD-3-Clause */
/*
 * Copyright (c) 2025, STMicroelectronics
 */

#ifndef __DRIVERS_STM32_DBGMCU_MBX_H
#define __DRIVERS_STM32_DBGMCU_MBX_H

#include <tee_api_types.h>

/*
 * Read a value from DBGMCU_DBG_AUTH_HOST register.
 * @value: pointer to value read from register
 * @timeout_ms: timeout in milliseconds
 * Return a TEE_Result compliant status
 */
TEE_Result stm32_dbgmcu_mbx_read_auth_host(uint32_t *value,
					   uint32_t timeout_ms);

/*
 * Write a value in DBGMCU_DBG_AUTH_DEV register.
 * @value: value to write in the register
 * @timeout_ms: timeout in milliseconds
 * Return a TEE_Result compliant status
 */
TEE_Result stm32_dbgmcu_mbx_write_auth_dev(uint32_t value, uint32_t timeout_ms);

#endif /* __DRIVERS_STM32_DBGMCU_MBX_H */
