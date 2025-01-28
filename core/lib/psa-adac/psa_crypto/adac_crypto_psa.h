/*
 * Copyright (c) 2020-2023, Arm Limited. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 */

#ifndef __ADAC_CRYPTO_PSA_H__
#define __ADAC_CRYPTO_PSA_H__

#include "psa_adac_config.h"
#include "psa_adac.h"
#include "psa/crypto.h"
#include "psa_adac_crypto_api.h"
#include "psa_adac_cryptosystems.h"

#ifdef __cplusplus
extern "C" {
#endif

psa_status_t psa_adac_verify_vendor(uint8_t key_type,
                                    uint8_t *key,
                                    size_t key_size,
                                    psa_algorithm_t hash_algo,
                                    const uint8_t *inputs[],
                                    size_t input_sizes[],
                                    size_t input_count,
                                    psa_algorithm_t sig_algo,
                                    uint8_t *sig,
                                    size_t sig_size);

psa_status_t psa_adac_verify_mac(uint8_t key_type,
                                 uint8_t *key,
                                 size_t key_size,
                                 const uint8_t *inputs[],
                                 size_t input_sizes[],
                                 size_t input_count,
                                 psa_algorithm_t mac_algo,
                                 uint8_t *mac,
                                 size_t mac_size);

#ifdef __cplusplus
}
#endif

#endif /*__ADAC_CRYPTO_PSA_H__ */
