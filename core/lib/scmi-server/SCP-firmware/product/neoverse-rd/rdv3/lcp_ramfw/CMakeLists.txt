#
# Arm SCP/MCP Software
# Copyright (c) 2022-2025, Arm Limited and Contributors. All rights reserved.
#
# SPDX-License-Identifier: BSD-3-Clause
#

#
# Create the firmware target.
#

add_executable(rdv3-lcp-bl2)

# SCP_PLATFORM_VARIANT value for supported RD-V3 platform variants:
# - '0' for RD-V3 platform
# - '1' for RD-V3-Cfg1 platform
# - '2' for RD-V3-Cfg2 platform

if (SCP_PLATFORM_VARIANT STREQUAL "0")
      target_compile_definitions(rdv3-lcp-bl2
                                 PUBLIC -DPLATFORM_VARIANT=0)
elseif (SCP_PLATFORM_VARIANT STREQUAL "1")
      target_compile_definitions(rdv3-lcp-bl2
                                 PUBLIC -DPLATFORM_VARIANT=1)
elseif (SCP_PLATFORM_VARIANT STREQUAL "2")
      target_compile_definitions(rdv3-lcp-bl2
                                 PUBLIC -DPLATFORM_VARIANT=2)
else()
      message(FATAL_ERROR "Please set SCP_PLATFORM_VARIANT to \n"
                                "'0' for RD-V3\n"
                                "'1' for RD-V3-Cfg1\n"
                                "'2' for RD-V3-Cfg2\n")
endif()

target_include_directories(
    rdv3-lcp-bl2 PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}"
                    "${CMAKE_CURRENT_SOURCE_DIR}/include")

# cmake-lint: disable=E1122

target_sources(
    rdv3-lcp-bl2
    PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/config_armv8m_mpu.c"
            "${CMAKE_CURRENT_SOURCE_DIR}/config_pl011.c"
            "${CMAKE_CURRENT_SOURCE_DIR}/config_transport.c"
            "${CMAKE_CURRENT_SOURCE_DIR}/config_mhu3.c"
            "${CMAKE_CURRENT_SOURCE_DIR}/config_dvfs_handler.c")

#
# Some of our firmware includes require CMSIS.
#

target_link_libraries(rdv3-lcp-bl2 PUBLIC cmsis::core-m)

#
# We explicitly add the CMSIS include directories to our interface include
# directories. Each module target adds these include directories to their own,
# allowing them to include any firmware includes we expose.
#

target_include_directories(rdv3-lcp-bl2
    PUBLIC $<TARGET_PROPERTY:cmsis::core-m,INTERFACE_INCLUDE_DIRECTORIES>)
