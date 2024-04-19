#
# Arm SCP/MCP Software
# Copyright (c) 2022, Linaro Limited and Contributors. All rights reserved.
# Copyright (c) 2024, STMicroelectronics and the Contributors. All rights reserved.
#
# SPDX-License-Identifier: BSD-3-Clause
#

add_library(${SCP_MODULE_TARGET} SCP_MODULE)

target_include_directories(${SCP_MODULE_TARGET}
                           PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include
                                  ${SCP_TFM_PLAT_DIR}/core/include/
                                  ${SCP_TFM_PLAT_DIR}/lib/libutils/ext/include/
                                  ${SCP_TFM_PLAT_DIR}/lib/libutee/include/
                                  ${SCP_TFM_DIR}/platform/ext/cmsis)

target_sources(${SCP_MODULE_TARGET}
               PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/src/mod_tfm_reset.c")

target_link_libraries(${SCP_MODULE_TARGET}
                      PRIVATE module-reset-domain)
