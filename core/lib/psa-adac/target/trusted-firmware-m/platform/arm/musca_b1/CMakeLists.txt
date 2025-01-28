#
# Copyright (c) 2023 Arm Limited. All rights reserved.
# SPDX-License-Identifier: BSD-3-Clause
#

target_sources(${PROJECT_NAME}
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/psa_adac_platform.c
        ${CMAKE_CURRENT_SOURCE_DIR}/discovery.c
        ${PSA_ADAC_ROOT}/transport_layer/transports/static_buffer_msg.c
        ${PSA_ADAC_ROOT}/transport_layer/transports/memory_window.c
)

target_include_directories(${PROJECT_NAME}
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/include
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}
        ${PSA_ADAC_MBEDTLS_INCLUDE}
        ${PSA_ADAC_ROOT}/transport_layer/transports
)

install(FILES ${CMAKE_CURRENT_SOURCE_DIR}/include/psa_adac_platform.h
        DESTINATION ${PSA_ADAC_INSTALL_PATH}/include
)
