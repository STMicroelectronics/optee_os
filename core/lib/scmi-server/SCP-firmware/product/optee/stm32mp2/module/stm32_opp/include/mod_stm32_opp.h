/*
 * Arm SCP/MCP Software
 * Copyright (c) 2025, STMicroelectronics and the Contributors. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef MOD_STM32_OPP_H
#define MOD_STM32_OPP_H

#include <fwk_element.h>
#include <fwk_macros.h>

#include <stdint.h>
#include <stdbool.h>

/*
 * the OPP identifier, used to identify the OPP instance start from 1
 * 0 is reserved for invalid OPP identifier
 */
#define OPP_ID_INVALID      0

/*!
 * \brief Platform STM32 OPP configuration.
 */
struct mod_stm32_opp_config {
    /*! OPP instance identifier */
    unsigned int opp_id;
};

#endif /* MOD_STM32_OPP_H */
