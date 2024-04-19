/*
 * Copyright (c) 2024, STMicroelectronics and the Contributors. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef MOD_STM32_PD_H
#define MOD_STM32_PD_H

/*!
 * \brief API indices.
 */
enum mod_stm32_pd_api_idx {
    MOD_STM32_PD_API_IDX_BASIC,
    MOD_STM32_PD_API_IDX_GPU,
    MOD_STM32_PD_API_IDX_COUNT
};

/*!
 * \brief Platform power domain configuration.
 */
struct mod_stm32_pd_config {
	const char *name;
	struct clk *clk;
	const struct device *regu;
	const struct firewall_spec *firewall;
	int n_firewall;
};

#endif /* MOD_STM32_PD_H */
