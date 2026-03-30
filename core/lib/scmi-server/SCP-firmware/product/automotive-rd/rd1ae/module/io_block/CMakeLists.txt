#
# Arm SCP/MCP Software
# Copyright (c) 2024, Arm Limited and Contributors. All rights reserved.
#
# SPDX-License-Identifier: BSD-3-Clause
#
add_library(${SCP_MODULE_TARGET} SCP_MODULE)

target_include_directories(${SCP_MODULE_TARGET}
    PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/include"
    PUBLIC "${CMAKE_SOURCE_DIR}/product/automotive-rd/rd1ae/interface/io_block/"
    PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/src")

target_sources(${SCP_MODULE_TARGET}
    PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/src/mod_io_block.c")

target_link_libraries(${SCP_MODULE_TARGET} PRIVATE module-noc-s3 module-atu)
