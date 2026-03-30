/*
 * Arm SCP/MCP Software
 * Copyright (c) 2025, Arm Limited and Contributors. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 * Description:
 *      SCMI power capping and monitoring protocol completer core.
 */

#ifndef INTERNAL_SCMI_POWER_CAPPING_CORE_H
#define INTERNAL_SCMI_POWER_CAPPING_CORE_H

#include "internal/scmi_power_capping.h"

#include <fwk_id.h>

#include <stdint.h>

#define SCMI_POWER_CAPPING_DISABLE_CAP_VALUE ((uint32_t)0)

int pcapping_core_set_cap(
    fwk_id_t service_id,
    unsigned int domain_idx,
    bool async_flag,
    uint32_t cap);

int pcapping_core_get_cap(unsigned int domain_idx, uint32_t *cap);

int pcapping_core_set_pai(
    fwk_id_t service_id,
    unsigned int domain_idx,
    uint32_t pai);

int pcapping_core_get_pai(unsigned int domain_idx, uint32_t *pai);

int pcapping_core_get_power(unsigned int domain_idx, uint32_t *power);

int pcapping_core_set_power_thresholds(
    unsigned int domain_idx,
    uint32_t threshold_low,
    uint32_t threshold_high);
struct pcapping_core_cap_pai_event_parameters {
    fwk_id_t service_id;
    uint32_t domain_idx;
    uint32_t cap;
    uint32_t pai;
};

#ifdef BUILD_HAS_SCMI_NOTIFICATIONS
struct pcapping_core_pwr_meas_event_parameters {
    fwk_id_t service_id;
    uint32_t domain_idx;
    uint32_t power;
};
#endif

int pcapping_core_get_cap_support(uint32_t domain_idx, bool *support);

int pcapping_core_get_pai_info(
    unsigned int domain_idx,
    uint32_t *min_pai,
    uint32_t *max_pai,
    uint32_t *pai_step);

int pcapping_core_get_config(
    unsigned int domain_idx,
    const struct mod_scmi_power_capping_domain_config **config);

unsigned int pcapping_core_get_domain_count(void);

int pcapping_core_start(unsigned int domain_idx);

int pcapping_core_bind(void);

void pcapping_core_init(unsigned int element_count);

int pcapping_core_domain_init(
    uint32_t domain_idx,
    const struct mod_scmi_power_capping_domain_config *config);

int pcapping_core_process_fwk_notification(
    const struct fwk_event *fwk_notification_event);

bool pcapping_core_is_cap_request_async(uint32_t domain_idx);

#endif /* INTERNAL_SCMI_POWER_CAPPING_CORE_H */
