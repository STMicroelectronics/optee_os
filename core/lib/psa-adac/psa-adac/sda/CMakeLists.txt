#
# Copyright (c) 2020-2024 Arm Limited. All rights reserved.
# SPDX-License-Identifier: BSD-3-Clause
#

add_library(psa_adac_sda STATIC)

target_include_directories(psa_adac_sda
    PUBLIC
        ${PSA_ADAC_ROOT}/psa-adac/sda/include
    PRIVATE
        ${PSA_ADAC_MBEDTLS_INCLUDE}

)

list(APPEND PSA_ADAC_SDA_SRC
    ${PSA_ADAC_ROOT}/psa-adac/sda/src/psa_adac_sda.c
)

target_sources(psa_adac_sda
    PUBLIC
        ${PSA_ADAC_SDA_SRC}
)

target_link_libraries(psa_adac_sda
    PRIVATE
        psa_adac_core
        ${PROJECT_NAME}
    PUBLIC
        psa_crypto_config
)

install(DIRECTORY
        ${CMAKE_CURRENT_SOURCE_DIR}/include/
    DESTINATION ${PSA_ADAC_INSTALL_PATH}/include
)
