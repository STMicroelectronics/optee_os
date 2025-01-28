#
# Copyright (c) 2020-2023 Arm Limited. All rights reserved.
# SPDX-License-Identifier: BSD-3-Clause
#

add_library(psa_adac_core STATIC)

configure_file(${CMAKE_CURRENT_SOURCE_DIR}/include/psa_adac_config.h.in ${CMAKE_CURRENT_SOURCE_DIR}/include/psa_adac_config.h)

target_include_directories(psa_adac_core
    PUBLIC
        ${PSA_ADAC_ROOT}/psa-adac/core/include
    PRIVATE
        ${PSA_ADAC_MBEDTLS_INCLUDE}
        ${PSA_ADAC_ROOT}/transport_layer/include
)

list(APPEND PSA_ADAC_CORE_SRC
        ${PSA_ADAC_ROOT}/psa-adac/core/src/adac_certificate.c
        ${PSA_ADAC_ROOT}/psa-adac/core/src/adac_crypto.c
        ${PSA_ADAC_ROOT}/psa-adac/core/src/adac_token.c)

target_sources(psa_adac_core
    PRIVATE
        ${PSA_ADAC_CORE_SRC}
)

if (DEFINED PSA_ADAC_TARGET)
    target_link_libraries(psa_adac_core
        PRIVATE
            ${PROJECT_NAME}
    )
endif()

install(DIRECTORY
        ${CMAKE_CURRENT_SOURCE_DIR}/include/
    DESTINATION ${PSA_ADAC_INSTALL_PATH}/include
)

add_library(psa_adac_config INTERFACE)

target_include_directories(psa_adac_config
    INTERFACE
        ${CMAKE_CURRENT_SOURCE_DIR}/include/
)
