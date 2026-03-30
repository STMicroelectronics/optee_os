#
# Arm SCP/MCP Software
# Copyright (c) 2025, Arm Limited and Contributors. All rights reserved.
#
# SPDX-License-Identifier: BSD-3-Clause
#
add_library(${SCP_MODULE_TARGET} SCP_MODULE)

# SCP_PLATFORM_VARIANT value for supported RD-V3 platform variants:
# - '0' for RD-V3 platform
# - '1' for RD-V3-Cfg1 platform
# - '2' for RD-V3-Cfg2 platform

if (SCP_PLATFORM_VARIANT STREQUAL "0")
     target_compile_definitions(${SCP_MODULE_TARGET}
                                 PUBLIC -DPLATFORM_VARIANT=0)
elseif (SCP_PLATFORM_VARIANT STREQUAL "1")
      target_compile_definitions(${SCP_MODULE_TARGET}
                                 PUBLIC -DPLATFORM_VARIANT=1)
elseif (SCP_PLATFORM_VARIANT STREQUAL "2")
      target_compile_definitions(${SCP_MODULE_TARGET}
                                 PUBLIC -DPLATFORM_VARIANT=2)
else()
      message(FATAL_ERROR "Please set SCP_PLATFORM_VARIANT to \n"
                                "'0' for RD-V3\n"
                                "'1' for RD-V3-Cfg1\n"
                                "'2' for RD-V3-Cfg2\n")
endif()

target_include_directories(${SCP_MODULE_TARGET}
                           PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/include")

target_sources(
    ${SCP_MODULE_TARGET}
    PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/src/mod_scp_platform.c"
    PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/src/platform_lcp.c"
    PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/src/platform_power_mgmt.c"
    PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/src/platform_rss.c")

target_link_libraries(
    ${SCP_MODULE_TARGET}
    PRIVATE module-power-domain module-system-power module-scmi
            module-system-info module-transport module-timer)
