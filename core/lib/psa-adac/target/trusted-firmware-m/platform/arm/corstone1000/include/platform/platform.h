/*
 * Copyright (c) 2020-2023 Arm Limited. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef __PLATFORM_H__
#define __PLATFORM_H__

#include <stdint.h>
#include <stddef.h>
#include "psa_adac.h"

#ifdef __cplusplus
extern "C" {
#endif

#ifdef PSA_ADAC_PLATFORM_CONFIG_FILE
#include PSA_ADAC_PLATFORM_CONFIG_FILE
#else
#include "psa_adac_platform.h"
#endif

#ifndef PSA_ADAC_PLATFORM_BANNER
#define PSA_ADAC_PLATFORM_BANNER "PSA ADAC "
#endif

void platform_init(void);
adac_status_t psa_adac_change_life_cycle_state(uint8_t *input, size_t input_size);
void psa_adac_platform_lock(void);
void psa_adac_platform_init(void);
int psa_adac_detect_debug_request(void);
void psa_adac_acknowledge_debug_request(void);

/**
 * \brief This function is called on response to the discovery command from the
 *        debug host. It returns information about the target and set of all
 *        response fragments format supported by the debug target.
 *
 * \param[out] reply             Pointer to \p reply buffer.
 * \param[in]  reply_size        Size of the \p reply buffer in bytes.
 *
 * \retval Returns size of actual populated reply buffer.
 */
size_t psa_adac_platform_discovery(uint8_t *reply, size_t reply_size);

int psa_adac_platform_check_token(uint8_t *token, size_t token_size);
int psa_adac_platform_check_certificate(uint8_t *crt, size_t crt_size);
int psa_adac_apply_permissions(uint8_t permissions_mask[16]);

#ifdef __cplusplus
}
#endif

#endif /* __PLATFORM_H__ */
