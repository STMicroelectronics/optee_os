/*
 * Arm SCP/MCP Software
 * Copyright (c) 2024-2025, Arm Limited and Contributors. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */
#ifndef PERF_CONTROLLER_H_
#define PERF_CONTROLLER_H_

#include <mod_perf_controller.h>

#include <fwk_id.h>

/*!
 * \brief Performance controller limiter context.
 *
 * \details The performance controller limiter context is responsible for
 * storing the power limit for a corresponding limiter.
 */
struct mod_perf_controller_limiter_ctx {
    uint32_t power_limit;
};

/*!
 * \brief Performance controller domain context.
 *
 * \details The performance controller domain is responsible for storing the
 *      aggregate information about the limiters.
 */
struct mod_perf_controller_domain_ctx {
    /*! Power model that converts from a power quantity to performance level. */
    const struct mod_perf_controller_power_model_api *power_model_api;

    /*! Performance driver API. */
    const struct mod_perf_controller_drv_api *perf_driver_api;

    /*! Performance limit for the domain. */
    uint32_t performance_limit;

    /*! Requested performance details for the domain. */
    struct {
        /*! Requested performance level. */
        uint32_t level;
        /*! Cookie assosiated with the request. */
        uintptr_t cookie;
    } performance_request_details;

    /*! Domain configuration. */
    const struct mod_perf_controller_domain_config *config;

    /*! Context table of limiters. */
    struct mod_perf_controller_limiter_ctx *limiter_ctx_table;

    /*! Number of limiters in the domain. */
    unsigned int limiter_count;

    /*! Notification count. */
    unsigned int notification_count;
};

/*!
 * \brief Performance controller module list of internal functions pointers.
 *
 * \details Internal functions are used via a function pointer call to make the
 *          unit testing of each function easier.
 */
struct mod_perf_controller_internal_api {
    /*! Memeber function to return minimum power limit. */
    uint32_t (*get_limiters_min_power_limit)(
        struct mod_perf_controller_domain_ctx *);

    /*! Memeber function to apply performance granted to the domain. */
    int (*domain_apply_performance_granted)(
        struct mod_perf_controller_domain_ctx *domain_ctx);
};

/*!
 * \brief Module context
 */
struct mod_perf_controller_ctx {
    /*! Context table of domains. */
    struct mod_perf_controller_domain_ctx *domain_ctx_table;

    /*! Number of domains in the module. */
    unsigned int domain_count;
};

#endif /* PERF_CONTROLLER_H_ */
