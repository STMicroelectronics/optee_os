/*
 * Copyright (c) 2024, STMicroelectronics and the Contributors. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef MOD_STM32_PD_H
#define MOD_STM32_PD_H

/*!
 * \brief Platform power domain configuration.
 */
struct mod_stm32_pd_config {
    const char *name;
    struct clk *clk;
    struct regulator *regu;
};

#endif /* MOD_STM32_PD_H */
