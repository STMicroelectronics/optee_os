/*
 * Arm SCP/MCP Software
 * Copyright (c) 2024-2025, Arm Limited and Contributors. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 * Description:
 *      Module scmi Pin Control Protocol.
 */
#ifndef MOD_SCMI_PIN_CONTROL_H
#define MOD_SCMI_PIN_CONTROL_H

#include <stddef.h>
#include <stdint.h>

/*!
 * \brief Configuration domain. Module can have multiple domains.
 */
struct config_domain {
    /*! Starting identifier of the domain */
    uint16_t identifier_begin;
    /*! Ending identifier of the domain */
    uint16_t identifier_end;
    /*! Determines how the identifiers are shifted */
    int32_t shift_factor;
};

/*!
 * \brief Module configuration.
 */
struct mod_scmi_pinctrl_domain_config {
    /*!
     * \brief Pointer to the table of domain configs.
     */
    const struct config_domain *config_domains;

    /*! Number of config domains */
    const size_t config_domain_count;
};

#endif /* MOD_SCMI_PIN_CONTROL_H */
