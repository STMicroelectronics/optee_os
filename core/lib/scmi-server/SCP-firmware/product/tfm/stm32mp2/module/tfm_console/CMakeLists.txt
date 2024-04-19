#
# Arm SCP/MCP Software
# Copyright (c) 2024, STMicroelectronics and the Contributors. All rights reserved.
#
# SPDX-License-Identifier: BSD-3-Clause
#

add_library(${SCP_MODULE_TARGET} SCP_MODULE)

target_include_directories(${SCP_MODULE_TARGET}
                           PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include
                                  ${SCP_TFM_DIR}/platform/ext/common/)

# These include directories are relative to SCP-firmware submodule position in
# tfm tree

target_sources(${SCP_MODULE_TARGET}
               PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/src/mod_tfm_console.c")
