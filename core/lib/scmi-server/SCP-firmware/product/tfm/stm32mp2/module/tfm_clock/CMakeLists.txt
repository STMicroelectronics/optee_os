#
# Arm SCP/MCP Software
# Copyright (c) 2024, STMicroelectronics and the Contributors. All rights reserved.
#
# SPDX-License-Identifier: BSD-3-Clause
#

add_library(${SCP_MODULE_TARGET} SCP_MODULE)

target_include_directories(${SCP_MODULE_TARGET}
	PUBLIC
		${CMAKE_CURRENT_SOURCE_DIR}/include
)

# These include directories are relative to SCP-firmware submodule position in
# tfm-os tree
target_include_directories(${SCP_MODULE_TARGET}
	PUBLIC
		${SCP_TFM_PLAT_DIR}/native_driver/include
                ${SCP_TFM_PLAT_DIR}/../devicetree/include
		${CMAKE_BINARY_DIR}/generated/devicetree/platform_s
		${SCP_TFM_DIR}/secure_fw/include
)

target_sources(${SCP_MODULE_TARGET}
        PRIVATE
		${CMAKE_CURRENT_SOURCE_DIR}/src/mod_tfm_clock.c
)

target_link_libraries(${SCP_MODULE_TARGET}
	PRIVATE
		module-clock
		platform_s
		tfm_config
)
