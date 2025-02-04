/* SPDX-License-Identifier: BSD-3-Clause */
/*
 * Copyright (c) 2019-2022, Arm Limited. All rights reserved.
 *
 */

/***********  WARNING: This is an auto-generated file. Do not edit!  **********/

#ifndef __PSA_MANIFEST_SID_H__
#define __PSA_MANIFEST_SID_H__

#ifdef __cplusplus
extern "C" {
#endif

	/******** RSE_NS_MAILBOX_AGENT ********/
#define RSE_MBOX_SERVICE_SID                                       (0x00000F00U)
#define RSE_MBOX_SERVICE_VERSION                                   (1U)
#define RSE_MBOX_SERVICE_HANDLE                                    (0x40000104U)

	/******** RSE_SP_SCP ********/
#define RSE_SCP_SERVICE_SID                                        (0x000000F0U)
#define RSE_SCP_SERVICE_VERSION                                    (1U)
#define RSE_SCP_SERVICE_HANDLE                                     (0x40000106U)

	/******** RSE_SP_PS ********/
#define RSE_PROTECTED_STORAGE_SERVICE_SID                          (0x00000060U)
#define RSE_PROTECTED_STORAGE_SERVICE_VERSION                      (1U)
#define RSE_PROTECTED_STORAGE_SERVICE_HANDLE                       (0x40000101U)

	/******** RSE_SP_ITS ********/
#define RSE_INTERNAL_TRUSTED_STORAGE_SERVICE_SID                   (0x00000070U)
#define RSE_INTERNAL_TRUSTED_STORAGE_SERVICE_VERSION               (1U)
#define RSE_INTERNAL_TRUSTED_STORAGE_SERVICE_HANDLE                (0x40000102U)

	/******** RSE_SP_CRYPTO ********/
#define RSE_CRYPTO_SID                                             (0x00000080U)
#define RSE_CRYPTO_VERSION                                         (1U)
#define RSE_CRYPTO_HANDLE                                          (0x40000100U)

	/******** RSE_SP_PLATFORM ********/
#define RSE_PLATFORM_SERVICE_SID                                   (0x00000040U)
#define RSE_PLATFORM_SERVICE_VERSION                               (1U)
#define RSE_PLATFORM_SERVICE_HANDLE                                (0x40000105U)

	/******** RSE_SP_INITIAL_ATTESTATION ********/
#define RSE_ATTESTATION_SERVICE_SID                                (0x00000020U)
#define RSE_ATTESTATION_SERVICE_VERSION                            (1U)
#define RSE_ATTESTATION_SERVICE_HANDLE                             (0x40000103U)

#ifdef __cplusplus
}
#endif

#endif /* __PSA_MANIFEST_SID_H__ */
