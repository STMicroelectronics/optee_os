#
# Arm SCP/MCP Software
# Copyright (c) 2024-2025, Arm Limited and Contributors. All rights reserved.
#
# SPDX-License-Identifier: BSD-3-Clause
#

if(CMAKE_C_COMPILER_ID STREQUAL "ARMClang")
    message(FATAL_ERROR "Arm Clang is not supported for the aarch64 architecture")
endif()

add_library(arch-aarch64)

target_include_directories(arch-aarch64
                           PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/include"
                                  "${CMAKE_CURRENT_SOURCE_DIR}/../common/include")

target_sources(
    arch-aarch64
    PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/src/arch_crt0.S"
            "${CMAKE_CURRENT_SOURCE_DIR}/src/arch_exceptions.S"
            "${CMAKE_CURRENT_SOURCE_DIR}/src/arch_gic.c"
            "${CMAKE_CURRENT_SOURCE_DIR}/src/arch_main.c"
            "${CMAKE_CURRENT_SOURCE_DIR}/../common/src/arch_mm.c"
            )

if(DEFINED SCP_AARCH64_PROCESSOR_TARGET)
    if(NOT SCP_AARCH64_PROCESSOR_TARGET MATCHES "cortex-r82")
        target_compile_definitions(arch-aarch64 PUBLIC "ARMV8A64_EL3_SUPPORT")
    endif()
endif()

if(SCP_HAVE_NEWLIB)
    target_compile_definitions(arch-aarch64 PUBLIC -DUSE_NEWLIB)
    target_sources(arch-aarch64
                   PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/../common/src/arch_libc_hooks.c")
    target_link_options(arch-aarch64 PUBLIC
        "LINKER:--undefined=_sbrk"
        "LINKER:--undefined=_kill")
endif()

include(SCPTargetLinkerScript)
scp_target_linker_script(arch-aarch64 "${CMAKE_CURRENT_SOURCE_DIR}/../common/src/arch.ld.S")
