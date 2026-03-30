/*
 * Arm SCP/MCP Software
 * Copyright (c) 2024, Arm Limited and Contributors. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef MOD_SYSTEM_PIK_H
#define MOD_SYSTEM_PIK_H

#include "system_pik.h"

#include <fwk_attributes.h>
#include <fwk_id.h>

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

/*!
 * \brief System PIK module configuration data.
 */
struct mod_system_pik_device_config {
    /*! Base address of the system PIK register block */
    uintptr_t system_pik_base;

    /*! Overide signal for Level 0 GPT entry size (GPCCR_EL3.L0GPTSZ) */
    uint32_t l0_gpt_size;
};

/*!
 * \}
 */

#endif /* MOD_SYSTEM_PIK_H */
