#
# Arm SCP/MCP Software
# Copyright (c) 2024-2025, Arm Limited and Contributors. All rights reserved.
#
# SPDX-License-Identifier: BSD-3-Clause
#

add_executable(fvp-baser-aemv8r)

target_include_directories(fvp-baser-aemv8r PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}")
target_sources(fvp-baser-aemv8r PRIVATE
    "${CMAKE_CURRENT_SOURCE_DIR}/config_pl011.c"
    "${CMAKE_CURRENT_SOURCE_DIR}/config_armv8r_mpu.c"
    "${CMAKE_CURRENT_SOURCE_DIR}/config_gicx00.c"
    "${CMAKE_CURRENT_SOURCE_DIR}/config_sp805.c"
    "${CMAKE_CURRENT_SOURCE_DIR}/config_timer.c"
    "${CMAKE_CURRENT_SOURCE_DIR}/config_gtimer.c"
    )

if(SCP_ENABLE_DEBUGGER)
    target_compile_definitions(fvp-baser-aemv8r PRIVATE BUILD_HAS_DEBUGGER)
    target_sources(fvp-baser-aemv8r PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/config_debugger_cli.c"
    "${CMAKE_CURRENT_SOURCE_DIR}/config_integration_test.c"
    "${CMAKE_CURRENT_SOURCE_DIR}/config_test_timer.c")
endif()
