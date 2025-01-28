#
# Copyright (c) 2022 Arm Limited. All rights reserved.
# SPDX-License-Identifier: BSD-3-Clause
#

set(ARM_SYSTEM_PROCESSOR cortex-m55)
set(ARM_SYSTEM_ARCHITECTURE armv8.1-m.main)
set(ARM_SYSTEM_FP OFF)

set(PSA_ADAC_EC_P256 ON CACHE BOOL "Enable support for ECDSA P-256")
set(PSA_ADAC_EC_P521 OFF CACHE BOOL "Enable support for ECDSA P-521")
set(PSA_ADAC_HW_CRYPTO ON CACHE BOOL "Support for hardware cryptography")
set(PSA_ADAC_AS_TFM_RUNTIME_SERVICE ON CACHE BOOL "Integrate ADAC as TF-M runtime service")

set(PSA_ADAC_USE_CRYPTOCELL On)
