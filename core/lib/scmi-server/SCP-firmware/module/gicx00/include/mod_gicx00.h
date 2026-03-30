/*
 * Arm SCP/MCP Software
 * Copyright (c) 2024, Arm Limited and Contributors. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef MOD_GICX00_H
#define MOD_GICX00_H

#include <stdint.h>

/*!
 * \ingroup GroupModules
 * \addtogroup GroupGICx00 Arm GICx00
 * \{
 */

/*!
 * \brief Module configuration.
 */
struct mod_gicx00_config {
    /*!
     * \brief GIC distributor base address
     */
    uintptr_t gicd_base;

    /*!
     * \brief GIC redistributor base address
     */
    uintptr_t gicr_base;
};

/*!
 * \}
 */

#endif /* MOD_GICX00_H */
