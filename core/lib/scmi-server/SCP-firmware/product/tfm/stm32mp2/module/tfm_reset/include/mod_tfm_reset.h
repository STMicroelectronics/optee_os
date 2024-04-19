/*
 * Arm SCP/MCP Software
 * Copyright (c) 2022, Linaro Limited and Contributors. All rights reserved.
 * Copyright (c) 2024, STMicroelectronics and the Contributors. All rights
 * reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef MOD_TFM_RESET_H
#define MOD_TFM_RESET_H

/*!
 * \brief Platform reset domain configuration.
 */
struct mod_tfm_reset_dev_config {
    /*! Optee reset reference */
    const struct reset_control *rstctrl;
    /*! Reset line name */
    const char *name;
};

#endif /* MOD_TFM_RESET_H */
