/*
 * Arm SCP/MCP Software
 * Copyright (c) 2025, Arm Limited and Contributors. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 * Description:
 *     SCMI PFDI Monitor
 */

#ifndef MOD_SCMI_PFDI_MONITOR_H
#define MOD_SCMI_PFDI_MONITOR_H

#include <fwk_id.h>

#include <stdint.h>

/*!
 * \brief SCMI PFDI monitor core configurations.
 */
struct mod_scmi_pfdi_monitor_core_config {
    fwk_id_t scmi_service_id;
    fwk_id_t pfdi_monitor_id;
};

#endif /* MOD_SCMI_PFDI_MONITOR_H */
