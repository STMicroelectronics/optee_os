#
# Arm SCP/MCP Software
# Copyright (c) 2021-2025, Arm Limited and Contributors. All rights reserved.
#
# SPDX-License-Identifier: BSD-3-Clause
#

add_executable(posix-host)

set(EXEC_TARGET posix-host)

target_include_directories(${EXEC_TARGET} PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}")
target_include_directories(${EXEC_TARGET} PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/../include")

target_sources(${EXEC_TARGET} PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/config_stdio.c")
target_sources(${EXEC_TARGET} PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/config_scmi.c")
target_sources(${EXEC_TARGET} PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/config_posix_mqueue.c")
target_sources(${EXEC_TARGET} PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/config_posix_transport.c")
