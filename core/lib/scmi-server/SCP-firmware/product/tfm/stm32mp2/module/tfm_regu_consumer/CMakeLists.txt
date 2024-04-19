#
# Arm SCP/MCP Software
# Copyright (c) 2023, Linaro Limited and Contributors. All rights reserved.
# Copyright (c) 2024, STMicroelectronics and the Contributors. All rights reserved.
#
# SPDX-License-Identifier: BSD-3-Clause
#

add_library(${SCP_MODULE_TARGET} SCP_MODULE)

target_include_directories(${SCP_MODULE_TARGET}
                          PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include
                                 ${SCP_SOURCE_DIR}/module/scmi/include
                                 ${SCP_TFM_PLAT_DIR}/native_driver/include
                                 ${SCP_TFM_PLAT_DIR}/../devicetree/include
                                 ${SCP_TFM_PLAT_DIR}/native_driver
                                 ${CMAKE_BINARY_DIR}/generated/devicetree/platform_s
                                 ${SCP_TFM_PLAT_DIR}/../../include)

target_sources(
    ${SCP_MODULE_TARGET} PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/src/mod_tfm_regu_consumer.c")

target_link_libraries(${SCP_MODULE_TARGET} PUBLIC module-voltage-domain)
