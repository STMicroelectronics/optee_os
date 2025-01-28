/*
 * Copyright (c) 2020-2023, Arm Limited. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 */

#include <string.h>
#include "psa_adac.h"
#include "psa_adac_crypto_api.h"

psa_status_t psa_adac_hash(psa_algorithm_t alg,
                           const uint8_t *input,
                           size_t input_size,
                           uint8_t *hash,
                           size_t hash_size,
                           size_t *hash_length)
{
    return psa_adac_hash_multiple(alg, &input, &input_size, 1,
                                  hash, hash_size, hash_length);
}

psa_status_t psa_adac_hash_multiple(psa_algorithm_t alg,
                                    const uint8_t *inputs[],
                                    size_t input_sizes[],
                                    size_t input_count,
                                    uint8_t hash[],
                                    size_t hash_size,
                                    size_t *hash_length)
{
    psa_status_t status;
    psa_hash_operation_t hash_operation = PSA_HASH_OPERATION_INIT;
    if (PSA_ALG_IS_VENDOR_DEFINED(alg) != 0) {
        // TODO: Add support for extra algorithms
        status = PSA_ERROR_NOT_SUPPORTED;
    } else {
        status = psa_hash_setup(&hash_operation, alg);
        for (size_t i = 0; (i < input_count) && (status == PSA_SUCCESS); i++) {
            status = psa_hash_update(&hash_operation, inputs[i], input_sizes[i]);

        }
        if (PSA_SUCCESS == status) {
            status = psa_hash_finish(&hash_operation, hash, hash_size, hash_length);
        } else {
            /* Free all allocated context in case hashing operation fails */
            /* Return the failed error status to the callee */
            (void)psa_hash_abort(&hash_operation);
        }
    }

    return status;
}

psa_status_t psa_adac_hash_verify(psa_algorithm_t alg,
                                  const uint8_t input[],
                                  size_t input_size,
                                  uint8_t hash[],
                                  size_t hash_size)
{
    psa_status_t status;
    psa_hash_operation_t hash_operation = PSA_HASH_OPERATION_INIT;
    if (PSA_ALG_IS_VENDOR_DEFINED(alg) != 0) {
        // TODO: Add support for extra algorithms
        status = PSA_ERROR_NOT_SUPPORTED;
    } else {
        status = psa_hash_setup(&hash_operation, alg);
        if (PSA_SUCCESS == status) {
            status = psa_hash_update(&hash_operation, input, input_size);
        }

        if (PSA_SUCCESS == status) {
            status = psa_hash_verify(&hash_operation, hash, hash_size);
        }
    }

    return status;
}

static psa_status_t hash_check(const uint8_t *input_a,
                               size_t len_a,
                               const uint8_t *input_b,
                               size_t len_b)
{
    int32_t result = 1;

    if (len_a == len_b) {
        result = memcmp(input_b, input_a, len_a);
    }

    return (result == 0U) ? PSA_SUCCESS : PSA_ERROR_INVALID_SIGNATURE;
}

psa_status_t psa_adac_hash_verify_multiple(psa_algorithm_t alg,
                                           const uint8_t input[],
                                           size_t input_length,
                                           uint8_t *hash[],
                                           size_t hash_size[],
                                           size_t hash_count)
{
    psa_status_t status;
    psa_hash_operation_t hash_operation = PSA_HASH_OPERATION_INIT;
    uint8_t computed_hash[PSA_HASH_MAX_SIZE];
    size_t computed_hash_len;

    if (PSA_ALG_IS_VENDOR_DEFINED(alg) != 0) {
        // TODO: Add support for extra algorithms
        status = PSA_ERROR_NOT_SUPPORTED;
    } else {
        status = psa_hash_setup(&hash_operation, alg);
        if (PSA_SUCCESS == status) {
            status = psa_hash_update(&hash_operation, input, input_length);
        }
        if (PSA_SUCCESS == status) {
            status = psa_hash_finish(&hash_operation, computed_hash,
                                     sizeof(computed_hash), &computed_hash_len);
        }
        if (PSA_SUCCESS == status) {
            for (size_t i = 0; i < hash_count; i++) {
                status = hash_check(hash[i], hash_size[i], computed_hash, computed_hash_len);
                if (status == PSA_SUCCESS) {
                    break;
                }
            }
        }
    }

    return status;
}
