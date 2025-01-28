/* SPDX-License-Identifier: BSD-3-Clause */
/*
 * Copyright (c) 2025 Arm Limited. All rights reserved.
 * Copyright (c) 2025 STMicroelectronics - All Rights Reserved
 */

#ifndef __PSA_ADAC_PLATFORM_H__
#define __PSA_ADAC_PLATFORM_H__

#include <psa_adac_config.h>

#define PSA_ADAC_PLATFORM_BANNER "PSA ADAC: OPTEE-OS STM32 platform"

/**
 * psa_adac_start_secure_debug() - Wait for host debugger to initiate the
 *				   secure debug connection and perform the
 *				   secure debug authentication process.
 *				   From optee to psa-adac.
 *
 * Returns TEE_SUCCESS when authentication is successful, otherwise
 * TEE_ERROR_CANCEL when no secure debug connection is detected, or
 * TEE_ERROR_NOT_SUPPORTED when PSA-ADAC library initialization failed, or
 * TEE_ERROR_GENERIC/TEE_ERROR_OUT_OF_MEMORY when failing to get ROTPKH, or
 * TEE_ERROR_BAD_STATE when message interface initialization failed or
 * secure debug is not authenticated.
 */
int psa_adac_start_secure_debug(void);

/**
 * psa_adac_apply_secure_debug_permissions() - Apply secure debug permissions.
 *					       From psa-adac to optee.
 *
 * @permissions_mask: Debug access permissions requested by the debug host
 */
int psa_adac_apply_secure_debug_permissions(uint8_t permissions_mask[16]);

#endif /* __PSA_ADAC_PLATFORM_H__ */
