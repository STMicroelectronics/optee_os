#
# Arm SCP/MCP Software
# Copyright (c) 2025, Arm Limited and Contributors. All rights reserved.
#
# SPDX-License-Identifier: BSD-3-Clause
#

add_library(arch-posix)

target_include_directories(arch-posix
                           PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/include")

target_sources(
    arch-posix PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/src/arch_interrupt.c"
                      "${CMAKE_CURRENT_SOURCE_DIR}/src/arch_main.c")
