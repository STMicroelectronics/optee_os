/*
 * Arm SCP/MCP Software
 * Copyright (c) 2024-2025, Arm Limited and Contributors. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 * Description:
 *      Power Distributor
 */

#ifndef MOD_POWER_DISTRIBUTOR_H
#define MOD_POWER_DISTRIBUTOR_H

#include <fwk_id.h>

/*!
 * \addtogroup GroupModules Modules
 * \{
 */

/*!
 * \defgroup GroupPowerDistributor Power Budget Distributor
 * \{
 */

/*!
 * \brief A parent index definition for domains that have no parents to use.
 *        (root domain).
 */
#define MOD_POWER_DISTRIBUTOR_DOMAIN_PARENT_IDX_NONE (UINT32_MAX)

/*!
 * \brief API indices
 */
enum mod_power_distributor_api_idx {
    /*! Power Distributor API distribution idx */
    MOD_POWER_DISTRIBUTOR_API_IDX_DISTRIBUTION,
    /*! Power Distributor API power management idx */
    MOD_POWER_DISTRIBUTOR_API_IDX_POWER_MANAGEMENT,
    /*! Power Distributor API count */
    MOD_POWER_DISTRIBUTOR_API_IDX_COUNT,
};

/*!
 * \brief Power Distributor API
 *
 * \details Interface implements distribution functionality all domains.
 */
struct mod_power_distributor_api {
    /*!
     * \brief Distribute Power Budgets
     *
     * By calling this API, Distributor distributes power budgets
     * from the ultimate root of the system.
     *
     * \retval ::FWK_SUCCESS The operation succeeded.
     * \retval ::One of the standard framework status codes.
     */
    int (*system_power_distribute)(void);
    /*!
     * \brief Distribute Power Budgets given a root domain of a subtree
     *
     * By calling this API, Distributor distributes power budgets
     * from the given domain all the way down to the leaves.
     * \param subtree_root_id Identifier of the domain to start distribution
     *                        from.
     * \retval ::FWK_SUCCESS The operation succeeded.
     * \retval ::One of the standard framework status codes.
     */
    int (*subtree_power_distribute)(fwk_id_t subtree_root_id);
};

/*!
 * \brief Power Distributor domain configuration data.
 */
struct mod_power_distributor_domain_config {
    /*! The parent domain index.
     * Set to MOD_POWER_DISTRIBUTOR_DOMAIN_PARENT_IDX_NONE
     * if no parent available.
     */
    uint32_t parent_idx;
    /*! Controller ID associated with the domain to apply the power budget. */
    fwk_optional_id_t controller_id;
    /*!
     * \brief Domain controller API ID
     *
     * \details The ID is only required when the domain
     *          has a controller that make use of the power budget.
     */
    fwk_optional_id_t controller_api_id;
};

/*!
 * \}
 */

/*!
 * \}
 */

#endif /* MOD_POWER_DISTRIBUTOR_H */
