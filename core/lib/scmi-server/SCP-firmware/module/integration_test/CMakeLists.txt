#
# Arm SCP/MCP Software
# Copyright (c) 2025, Arm Limited and Contributors. All rights reserved.
#
# SPDX-License-Identifier: BSD-3-Clause
#

set(CONTRIB_ROOT ${CMAKE_SOURCE_DIR}/contrib)
set(UNITY_SRC_ROOT ${CONTRIB_ROOT}/cmock/git/vendor/unity/src)
set(UNIT_TEST_ROOT ${CMAKE_SOURCE_DIR}/unit_test)

add_library(${SCP_MODULE_TARGET} SCP_MODULE)

target_include_directories(
    ${SCP_MODULE_TARGET}
    PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/include"
           "${UNIT_TEST_ROOT}/unity_mocks"
           ${CMOCK_SRC_ROOT}
           ${UNITY_SRC_ROOT})

target_sources(
    ${SCP_MODULE_TARGET}
     PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/src/mod_integration_test.c"
             "${UNITY_SRC_ROOT}/unity.c"
             "${UNIT_TEST_ROOT}/unity_mocks/scp_unity.c")

get_target_property(target_type ${SCP_MODULE_TARGET} TYPE)
if(NOT target_type STREQUAL "INTERFACE_LIBRARY")
    target_compile_definitions(
        ${SCP_MODULE_TARGET}
        PUBLIC "UNITY_INCLUDE_CONFIG_H")
endif()
