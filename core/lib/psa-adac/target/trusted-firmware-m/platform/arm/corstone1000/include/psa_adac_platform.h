/*
 * Copyright (c) 2021 Arm Limited. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef PSA_ADAC_TRUSTED_FIRMWARE_M_CORSTONE1000__PSA_ADAC_PLATFORM_H
#define PSA_ADAC_TRUSTED_FIRMWARE_M_CORSTONE1000_PSA_ADAC_PLATFORM_H

#include <psa_adac_config.h>

#define PSA_ADAC_PLATFORM_BANNER "PSA ADAC: Tresuted-Firmware-M Dipda platform."
#define PSA_ADAC_AUTHENTICATOR_IMPLICIT_TRANSPORT

/*
 * From tf-m to psa-adac.
 * Call to this function will wait for host debugger to initiate the
 * secure debug connection and will perform the secure debug authentication
 * proces.
 */
int tfm_to_psa_adac_corstone1000_secure_debug(uint8_t *secure_debug_rotpk, uint32_t len);

/*
 * From psa-adac to tfm
 * The platform code in the tf-m can use this function to apply
 * secure debug permissions.
 */
int psa_adac_to_tfm_apply_permissions(uint8_t permissions_mask[16]);


#endif //PSA_ADAC_TRUSTED_FIRMWARE_M_CORSTONE1000_PSA_ADAC_PLATFORM_H
