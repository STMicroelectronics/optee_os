/*
 * Copyright (c) 2020-2023 Arm Limited. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef __PSA_ADAC_PLATFORM_H__
#define __PSA_ADAC_PLATFORM_H__

#include "psa_adac_config.h"

#ifdef __cplusplus
extern "C" {
#endif

#define PSA_ADAC_PLATFORM_BANNER "PSA ADAC: Musca-B1 TF-M"
#define LED_R  0x04  /* LED RED (PA2) */
#define LED_G  0x08  /* LED GREEN (PA3) */
#define LED_B  0x10  /* LED BLUE (PA4) */

#define MUSCA_B1_SRAM_S_BASE (0x30000000UL)  /*!< (Secure Internal SRAM) Base Address */
#define MUSCA_B1_SRAM_S_SIZE (0x2000)        /*!< (Secure Internal SRAM) Size = 8 KiB */
#define SDM_MEMORY_WINDOW_BASE MUSCA_B1_SRAM_S_BASE
#define SDM_MEMORY_WINDOW_SIZE MUSCA_B1_SRAM_S_SIZE
#define PSA_ADAC_TRANSPORT_OWN_MEMORY
#define PSA_ADAC_AUTHENTICATOR_IMPLICIT_TRANSPORT

/*
 * From tf-m to psa-adac.
 * Call to this function will wait for host debugger to initiate the
 * secure debug connection and will perform the secure debug authentication
 * process.
 */
int tfm_to_psa_adac_musca_b1_secure_debug(uint8_t *secure_debug_rotpk, uint32_t len);

#ifdef __cplusplus
}
#endif

#endif /* __PSA_ADAC_PLATFORM_H__ */
