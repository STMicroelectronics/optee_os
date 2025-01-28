#
# Copyright (c) 2021 Arm Limited. All rights reserved.
# SPDX-License-Identifier: BSD-3-Clause
#

target_include_directories(${PROJECT_NAME}
    PRIVATE
        ${PSA_ADAC_MBEDTLS_INCLUDE}
)

set(MBEDTLS_CONFIG_FILE "${CMAKE_CURRENT_SOURCE_DIR}/authenticator-crypto-config.h")

add_compile_options($<$<COMPILE_LANGUAGE:C>:-DMBEDTLS_CONFIG_FILE="${MBEDTLS_CONFIG_FILE}">)

target_compile_definitions(${PROJECT_NAME}
    PUBLIC
        TRUSTED_FIRMWARE_M_PSA_ADAC
)

add_subdirectory(${TFM_PLATFORM_PATH})
