/* SPDX-License-Identifier: BSD-3-Clause */
/*
 * Copyright (c) 2022-2025 Arm Limited. All rights reserved.
 * Copyright (c) 2025 STMicroelectronics - All Rights Reserved
 */

#ifndef __PLATFORM_H__
#define __PLATFORM_H__

#include <stdint.h>
#include <stddef.h>

#include <psa_adac_platform.h>

void platform_init(void);
/**
 * psa_adac_platform_discovery() - This function is called on response to the
 *				   discovery command from the debug host.
 *				   It returns information about the target and
 *				   set of all response fragments format
 *				   supported by the debug target.
 *
 * @reply: [out] Pointer to reply buffer.
 * @reply_size: [in] Size of the reply buffer in bytes.
 *
 * Returns size of actual populated reply buffer.
 */
size_t psa_adac_platform_discovery(uint8_t *reply, size_t reply_size);
adac_status_t psa_adac_change_life_cycle_state(uint8_t *input,
					       size_t input_size);
void psa_adac_platform_lock(void);
void psa_adac_platform_init(void);
int psa_adac_platform_check_token(uint8_t *token, size_t token_size);
int psa_adac_platform_check_certificate(uint8_t *crt, size_t crt_size);
int psa_adac_apply_permissions(uint8_t permissions_mask[16]);

int psa_adac_detect_debug_request(void);
void psa_adac_acknowledge_debug_request(void);

#endif /* __PLATFORM_H__ */
