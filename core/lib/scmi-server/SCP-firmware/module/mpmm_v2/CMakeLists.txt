#
# Arm SCP/MCP Software
# Copyright (c) 2024, Arm Limited and Contributors. All rights reserved.
#
# SPDX-License-Identifier: BSD-3-Clause
#

add_library(${SCP_MODULE_TARGET} SCP_MODULE)

target_include_directories(${SCP_MODULE_TARGET}
                           PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/include")

target_include_directories(${SCP_MODULE_TARGET}
                           PUBLIC "${CMAKE_SOURCE_DIR}/interface/amu"
                           PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/../metrics_analyzer/include"
                           PUBLIC "${CMAKE_SOURCE_DIR}/interface/power_management")

target_sources(${SCP_MODULE_TARGET}
               PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/src/mod_mpmm_v2.c")

target_link_libraries(${SCP_MODULE_TARGET} PRIVATE module-power-domain)

if(BUILD_HAS_MOD_PERF_CONTROLLER)
target_link_libraries(${SCP_MODULE_TARGET} PRIVATE module-perf-controller)
endif()
