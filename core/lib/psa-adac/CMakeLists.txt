#
# Copyright (c) 2021 Arm Limited. All rights reserved.
# SPDX-License-Identifier: BSD-3-Clause
#

cmake_minimum_required(VERSION 3.15)

get_filename_component(PSA_ADAC_ROOT ${CMAKE_CURRENT_SOURCE_DIR} ABSOLUTE)
get_filename_component(PSA_ADAC_BINARY_DIR ${CMAKE_CURRENT_BINARY_DIR} ABSOLUTE)

if (NOT DEFINED PSA_ADAC_TARGET)
    Message(FATAL_ERROR "PSA_ADAC_TARGET not defined.")
endif()

get_filename_component(PSA_ADAC_TARGET_PATH ${CMAKE_CURRENT_SOURCE_DIR}/target/${PSA_ADAC_TARGET} ABSOLUTE)

if (NOT EXISTS ${PSA_ADAC_TARGET_PATH})
    Message(FATAL_ERROR "Target ${PSA_ADAC_TARGET_PATH} not supported.")
endif()

if (NOT DEFINED PSA_ADAC_MBEDTLS_INCLUDE)
    Message(FATAL_ERROR "PSA_ADAC_MBEDTLS_INCLUDE not defined.")
endif()

if (NOT EXISTS ${PSA_ADAC_MBEDTLS_INCLUDE})
    Message(FATAL_ERROR "Does not exist: ${PSA_ADAC_MBEDTLS_INCLUDE}")
endif()


get_filename_component(PSA_ADAC_INSTALL_PATH ${CMAKE_CURRENT_BINARY_DIR}/install ABSOLUTE)

# NOTE: cmake config priority is bottom to top, so dont change the order of
# below includes
include(${PSA_ADAC_TARGET_PATH}/config.cmake)
include(${PSA_ADAC_ROOT}/cmake/psa_adac.cmake)

if (${PSA_ADAC_TOOLCHAIN})
    include(${PSA_ADAC_ROOT}/cmake/toolchain.cmake)
endif()

project(${PSA_ADAC_TARGET}-psa-adac LANGUAGES C ASM)

if (${PSA_ADAC_TOOLCHAIN})
    arm_toolchain_reload_compiler()
endif()

add_library(${PROJECT_NAME} STATIC)

add_subdirectory(${PSA_ADAC_ROOT}/psa-adac/core adac_core)
add_subdirectory(${PSA_ADAC_ROOT}/psa-adac/sda adac_sda)
add_subdirectory(${PSA_ADAC_TARGET_PATH} adac_platform)
add_subdirectory(${PSA_ADAC_ROOT}/psa_crypto)

target_link_libraries(${PROJECT_NAME}
    PUBLIC
        psa_adac_core
        psa_adac_sda
)

install(FILES "${CMAKE_CURRENT_BINARY_DIR}/lib${PROJECT_NAME}.a"
        DESTINATION ${PSA_ADAC_INSTALL_PATH}/lib
)
