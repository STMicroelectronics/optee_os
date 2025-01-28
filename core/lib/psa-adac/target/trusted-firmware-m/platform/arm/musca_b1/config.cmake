#
# Copyright (c) 2023 Arm Limited. All rights reserved.
# SPDX-License-Identifier: BSD-3-Clause
#

set(ARM_SYSTEM_PROCESSOR cortex-m33)
set(ARM_SYSTEM_ARCHITECTURE armv8-m.main)
set(ARM_SYSTEM_DSP OFF)

set(PSA_ADAC_EC_P256 ON CACHE BOOL "Enable support for ECDSA P-256")
set(PSA_ADAC_EC_P521 OFF CACHE BOOL "Enable support for ECDSA P-521")
set(PSA_ADAC_HW_CRYPTO OFF CACHE BOOL "Support for hardware cryptography")
set(PSA_ADAC_USE_STATIC_PUB_KEYS ON CACHE BOOL "Use statically allocated buffer for public keys")

set(PSA_ADAC_USE_CRYPTOCELL On)
