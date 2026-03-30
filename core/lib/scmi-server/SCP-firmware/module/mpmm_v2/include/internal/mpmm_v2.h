/*
 * Arm SCP/MCP Software
 * Copyright (c) 2024, Arm Limited and Contributors. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef MPMM_REG_DEFS_H
#define MPMM_REG_DEFS_H

#include <mod_mpmm_v2.h>

#include <fwk_macros.h>

#include <stdint.h>

/*! MPMM registers */
struct mpmm_reg {
    /*! Global PPM(Performance and power management) feature register. */
    FWK_R uint32_t PPMCR;
    uint32_t RESERVED0[3];
    /*! MPMM control register. */
    FWK_RW uint32_t MPMMCR;
};

/* PPMCR bits definitions */
#define MPMM_PPMCR_NUM_GEARS_MASK UINT32_C(0x00000007)
#define MPMM_PPMCR_NUM_GEARS_POS  8

/* MPMMCR bits definitions */
#define MPMM_MPMMCR_EN_MASK   UINT32_C(0x00000001)
#define MPMM_MPMMCR_GEAR_MASK UINT32_C(0x00000003)
#define MPMM_MPMMCR_EN_POS    0
#define MPMM_MPMMCR_GEAR_POS  1

struct mod_mpmm_v2_core_ctx {
    /* Core Identifier */
    fwk_id_t core_id;

    /* MPMM registers */
    struct mpmm_reg *mpmm_v2;

    /* The core is online */
    bool online;

    /* Update only when the gear changes */
    bool needs_gear_update;

    /* Current selected gear */
    uint32_t selected_gear;

    /* Cached counters */
    uint64_t *cached_counters;

    /* Counters delta */
    uint64_t *delta;

    /* Used to block the PD when transitioning from OFF to ON */
    bool pd_blocked;

    /* Cookie to un-block the PD transition from OFF to ON */
    uint32_t cookie;

    /* Identifier of the base AMU Auxiliry counter */
    fwk_id_t base_aux_counter_id;

    /* The core is initialized */
    bool initialized;
};

struct mod_mpmm_v2_domain_ctx {
    /* Context Domain ID */
    fwk_id_t domain_id;

    /* Number of cores to monitor */
    uint32_t num_cores;

    /* Number of cores online */
    uint32_t num_cores_online;

    /* Latest power limit as reported by the perf Controller */
    uint32_t current_power_level;

    /* Latest power limit value required by MPMM */
    uint32_t power_limit;

    /* Wait for the report callback to confirm perf transition completion */
    bool wait_for_perf_transition;

    /* Core context */
    struct mod_mpmm_v2_core_ctx core_ctx[MPMM_MAX_NUM_CORES_IN_DOMAIN];

    /* Domain configuration */
    const struct mod_mpmm_v2_domain_config *domain_config;
};

#endif /* MPMM_REG_DEFS_H */
