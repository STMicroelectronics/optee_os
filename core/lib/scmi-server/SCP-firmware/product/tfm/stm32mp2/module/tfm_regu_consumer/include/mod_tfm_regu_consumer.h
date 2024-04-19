/*
 * Copyright (c) 2020, Arm Limited and Contributors. All rights reserved.
 * Copyright (c) 2024, STMicroelectronics
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef MOD_TFM_REGU_CONSUMER_H
#define MOD_TFM_REGU_CONSUMER_H

#include <fwk_element.h>
#include <fwk_macros.h>
#include <stdint.h>
#include <stdbool.h>

struct scmi_server_regu_channel;
struct rdev;

/*!
 * \brief Platform regulator configuration.
 */
struct mod_tfm_regu_consumer_dev_config {
    const struct device *dev;
    bool default_enabled;
};

#endif /* MOD_TFM_REGU_CONSUMER_H */
