/*
 * Copyright (c) 2022 Arm Limited. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef __PSA_ADAC_PLATFORM_H__
#define __PSA_ADAC_PLATFORM_H__

#include <psa_adac_config.h>

#ifdef __cplusplus
extern "C" {
#endif

#define PSA_ADAC_PLATFORM_BANNER "PSA ADAC: Trusted-Firmware-M RSE platform."
#define PSA_ADAC_AUTHENTICATOR_IMPLICIT_TRANSPORT

/*
 * From tf-m to psa-adac.
 * Call to this function will wait for host debugger to initiate the
 * secure debug connection and will perform the secure debug authentication
 * proces.
 */
int tfm_to_psa_adac_rse_secure_debug(uint8_t *secure_debug_rotpk, uint32_t len);

/*
 * From psa-adac to tfm
 * The platform code in the tf-m can use this function to apply
 * secure debug permissions.
 */
int psa_adac_to_tfm_apply_permissions(uint8_t permissions_mask[16]);

#ifdef __cplusplus
}
#endif

#endif /* __PSA_ADAC_PLATFORM_H__ */
