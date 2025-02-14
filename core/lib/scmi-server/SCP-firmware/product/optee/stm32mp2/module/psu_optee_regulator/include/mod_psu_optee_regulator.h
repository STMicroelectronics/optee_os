/*
 * Copyright (c) 2020, Arm Limited and Contributors. All rights reserved.
 * Copyright (c) 2023, STMicroelectronics
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef MOD_PSU_OPTEE_REGULATOR_H
#define MOD_PSU_OPTEE_REGULATOR_H

#include <fwk_element.h>
#include <fwk_macros.h>
#include <stdint.h>
#include <stdbool.h>

struct regulator;

/*!
 * \brief Platform regulator configuration.
 */
struct mod_psu_optee_regulator_dev_config {
    struct regulator *regulator;
};

#endif /* MOD_PSU_OPTEE_REGULATOR_H */
