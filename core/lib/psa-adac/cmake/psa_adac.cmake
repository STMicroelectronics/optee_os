#-------------------------------------------------------------------------------
# Copyright (c) 2020-2021, Arm Limited. All rights reserved.
#
# SPDX-License-Identifier: BSD-3-Clause
#
#-------------------------------------------------------------------------------


list(APPEND CMAKE_MODULE_PATH ${PSA_ADAC_ROOT}/cmake)

set(PSA_ADAC_TOOLCHAIN    TRUE    CACHE BOOL "Whether to use psa-adac toolchain.")

set(PSA_ADAC_EC_P256 On CACHE BOOL "Enable support for ECDSA P-256")
set(PSA_ADAC_EC_P521 On CACHE BOOL "Enable support for ECDSA P-521")
set(PSA_ADAC_RSA3072 On CACHE BOOL "Enable support for RSA 3072")
set(PSA_ADAC_RSA4096 On CACHE BOOL "Enable support for RSA 4096")
set(PSA_ADAC_ED25519 Off CACHE BOOL "Enable support for EdDSA Ed25519")
set(PSA_ADAC_ED448 Off CACHE BOOL "Enable support for EdDSA Ed448")
set(PSA_ADAC_SM2SM3 Off CACHE BOOL "Enable support for SM2/SM3")
set(PSA_ADAC_CMAC On CACHE BOOL "Enable support for CMAC AES-128")
set(PSA_ADAC_HMAC On CACHE BOOL "Enable support for HMAC SHA-256")
set(PSA_ADAC_HW_CRYPTO Off CACHE BOOL "Support for hardware cryptography")
set(PSA_ADAC_DEBUG Off CACHE BOOL "Enable debug")
set(PSA_ADAC_QUIET Off CACHE BOOL "Disable console output")
set(PSA_ADAC_QEMU Off CACHE BOOL "The image will be built to run on QEMU")
set(PSA_ADAC_MINIMUM_SIZE_CONFIG Off CACHE BOOL "Size-optimized build (reduced features)")

if (CMAKE_BUILD_TYPE STREQUAL "Debug")
    set(PSA_ADAC_DEBUG On)
    set(PSA_ADAC_TRACE Off)
elseif (CMAKE_BUILD_TYPE STREQUAL "MinSizeRel")
    set(PSA_ADAC_QUIET On)
    set(PSA_ADAC_DEBUG Off)
endif ()

if (ROM_POC) # TODO: this is just for transition, remove soon
    set(PSA_ADAC_MINIMUM_SIZE_CONFIG On)
endif ()

if (PSA_ADAC_MINIMUM_SIZE_CONFIG AND NOT (PLATFORM_NAME STREQUAL "native"))
    # set(PSA_ADAC_EC_P256 On)
    set(PSA_ADAC_EC_P521 Off)
    set(PSA_ADAC_RSA3072 Off)
    set(PSA_ADAC_RSA4096 Off)
    set(PSA_ADAC_ED25519 Off)
    set(PSA_ADAC_ED448 Off)
    set(PSA_ADAC_SM2SM3 Off)
    set(PSA_ADAC_CMAC Off)
    set(PSA_ADAC_HMAC Off)
endif ()

find_program(CCACHE_PROGRAM ccache)
if (CCACHE_PROGRAM)
    set_property(GLOBAL PROPERTY RULE_LAUNCH_COMPILE "${CCACHE_PROGRAM}")
endif ()
