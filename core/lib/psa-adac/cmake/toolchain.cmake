#-------------------------------------------------------------------------------
# Copyright (c) 2020-2021, Arm Limited. All rights reserved.
#
# SPDX-License-Identifier: BSD-3-Clause
#
#-------------------------------------------------------------------------------

set(CMAKE_C_COMPILER_FORCED true)
set(CROSS_COMPILE arm-none-eabi CACHE STRING "Cross-compilation triplet")

if (NOT (COMPILER))
    set(COMPILER "GNUARM")
endif ()

if (COMPILER STREQUAL "GNUARM")

    include(${PSA_ADAC_ROOT}/cmake/toolchain_GNUARM.cmake)

    if (CMAKE_BUILD_TYPE STREQUAL "MinSizeRel")
        add_compile_options(-flto -Os)
        add_link_options(-Wl,--as-needed -flto -flto-partition=none -Os -ffunction-sections -fuse-linker-plugin)
        add_compile_options($<$<COMPILE_LANGUAGE:C>:-DNDEBUG>)
    endif ()

elseif(COMPILER STREQUAL "ARMCLANG")

    include(${PSA_ADAC_ROOT}/cmake/toolchain_ARMCLANG.cmake)

    if (CMAKE_BUILD_TYPE STREQUAL "MinSizeRel")
        add_compile_options($<$<COMPILE_LANGUAGE:C>:-Oz>)
        # # Can't enable LTO for all targets because static libraries (like mbedcrypto)
        # # are not supported.
        # add_compile_options(-flto)
        # add_link_options(--lto)
        add_compile_options($<$<COMPILE_LANGUAGE:C>:-DNDEBUG>)
    endif ()

elseif(COMPILER STREQUAL "IARARM")

    include(${PSA_ADAC_ROOT}/cmake/toolchain_IARARM.cmake)

    if (CMAKE_BUILD_TYPE STREQUAL "MinSizeRel")
        add_compile_options(--mfc)
        add_link_options(--vfe)
        add_compile_options($<$<COMPILE_LANGUAGE:C>:-DNDEBUG>)
    endif()

else()

    message(FATAL_ERROR "\nValid values for COMPILER are 'GNUARM' (default), 'ARMCLANG' and 'IARARM'\n")

endif()
