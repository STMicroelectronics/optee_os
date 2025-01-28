#
# Copyright (c) 2021-2024 Arm Limited. All rights reserved.
# SPDX-License-Identifier: BSD-3-Clause
#
if (NOT DEFINED TFM_PLATFORM)
    Message(FATAL_ERROR "TFM_PLATFORM not defined.")
endif()

get_filename_component(TFM_PLATFORM_PATH ${CMAKE_CURRENT_SOURCE_DIR}/target/trusted-firmware-m/platform/${TFM_PLATFORM} ABSOLUTE)

if (NOT EXISTS ${TFM_PLATFORM_PATH})
    Message(FATAL_ERROR "Platform ${TFM_PLATFORM} not supported.")
endif()

include(${TFM_PLATFORM_PATH}/config.cmake)

set(PSA_ADAC_QUIET OFF CACHE BOOL "The image will be built to run on QEMU")
set(PSA_ADAC_DEBUG ON CACHE BOOL "Enable debug")
set(PSA_ADAC_AS_TFM_RUNTIME_SERVICE OFF CACHE BOOL "Integrate ADAC as TF-M runtime service")
