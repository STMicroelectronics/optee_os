#
# Copyright (c) 2022 Arm Limited. All rights reserved.
# SPDX-License-Identifier: BSD-3-Clause
#

set(TFM_SRC_DIR ${CMAKE_SOURCE_DIR})

target_sources(${PROJECT_NAME}
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/psa_adac_platform.c
        ${CMAKE_CURRENT_SOURCE_DIR}/discovery.c
        ${PSA_ADAC_ROOT}/transport_layer/transports/static_buffer_msg.c
        ${PSA_ADAC_ROOT}/transport_layer/transports/sdc-600/sdc-600.c
        ${PSA_ADAC_ROOT}/transport_layer/transports/sdc-600/int_com_port_driver.c
)

target_include_directories(${PROJECT_NAME}
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/include
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}
        ${PSA_ADAC_MBEDTLS_INCLUDE}
        ${PSA_ADAC_ROOT}/transport_layer/transports
        ${TFM_SRC_DIR}/platform/ext/target/arm/rse/common/partition
)

install(FILES
            ${CMAKE_CURRENT_SOURCE_DIR}/include/psa_adac_platform.h
        DESTINATION ${PSA_ADAC_INSTALL_PATH}/include
)
