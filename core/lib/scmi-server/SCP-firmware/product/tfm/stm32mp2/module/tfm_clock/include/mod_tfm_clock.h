/*
 * Arm SCP/MCP Software
 * Copyright (c) 2022, Linaro Limited and Contributors. All rights reserved.
 * Copyright (c) 2024, STMicroelectronics and the Contributors. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef MOD_TFM_CLOCK_H
#define MOD_TFM_CLOCK_H

#include <fwk_element.h>
#include <fwk_macros.h>

#include <clk.h>

#include <stdint.h>
#include <stdbool.h>

/*!
 * \brief Platform clocks configuration.
 */
struct mod_tfm_clock_config {
    /*! Clock name */
    const char *name;
    /*! Optee clock reference */
    const struct clk *clk;
    /*! default state of the clock */
    bool default_enabled;
};

#endif /* MOD_TFM_CLOCK_H */
