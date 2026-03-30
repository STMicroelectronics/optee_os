#
# Arm SCP/MCP Software
# Copyright (c) 2025, Arm Limited and Contributors. All rights reserved.
#
# SPDX-License-Identifier: BSD-3-Clause
#

add_library(${SCP_MODULE_TARGET} SCP_MODULE)

target_include_directories(${SCP_MODULE_TARGET}
                           PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/include"
                           PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/include/internal"
                           PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/src")

target_sources(
    ${SCP_MODULE_TARGET}
     PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/src/mod_ni_710ae.c"
     PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/src/ni_710ae_lib.c"
     PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/src/ni_710ae_discovery_drv.c"
     PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/src/ni_710ae_apu_drv.c")
