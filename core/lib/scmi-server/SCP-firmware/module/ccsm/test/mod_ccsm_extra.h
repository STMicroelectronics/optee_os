/*
 * Arm SCP/MCP Software
 * Copyright (c) 2024, Arm Limited and Contributors. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <ccsm_drv.h>

#include <mod_ccsm.h>

/* Driver functions*/

/*!
 * \brief Get PLL static configuration registers.
 *
 * \param reg Pointer to the CCSM register struct.
 *
 * \param pll_select Selector for physical PLL to read from.
 *
 * \param[out] config Current static configuration register value for the PLL.
 *
 * \retval ::FWK_SUCCESS The operation succeeded.
 * \retval ::FWK_E_PARAM The `pll_select` parameter was an invalid value.
 */
int ccsm_drv_get_pll_static_settings(
    struct ccsm_reg *reg,
    enum mod_ccsm_pll_static_select pll_select,
    uint32_t *config);

/*!
 * \brief Get PLL dynamic configuration registers.
 *
 * \param reg Pointer to the CCSM register struct.
 *
 * \param pll_select Selector for which PLL to read from, nominal or fallback.
 *
 * \param[out] config0 Current register value read for settings 0.
 *
 * \param[out] config1 Current register value read for settings 1.
 *
 * \retval ::FWK_SUCCESS The operation succeeded.
 * \retval ::FWK_E_PARAM The `pll_select` parameter was an invalid value.
 */
int ccsm_drv_get_pll_dynamic_settings(
    struct ccsm_reg *reg,
    enum mod_ccsm_pll_dynamic_select pll_select,
    uint32_t *config0,
    uint32_t *config1);

/*!
 * \brief Set static config for a PLL.
 *
 * \param reg Pointer to the CCSM register struct.
 *
 * \param pll_select Selector for which physical PLL to update.
 *
 * \param config The register value to be set for static config of PLL.
 *
 * \retval ::FWK_SUCCESS The operation succeeded.
 * \retval ::FWK_E_PARAM The `pll_select` parameter was an invalid value.
 */
int ccsm_drv_set_pll_static_settings(
    struct ccsm_reg *reg,
    enum mod_ccsm_pll_static_select pll_select,
    uint32_t config);

/*!
 * \brief Set dynamic config for a PLL.
 *
 * \param reg Pointer to the CCSM register struct.
 *
 * \param pll_select Selector for which PLL to update, nominal or fallback.
 *
 * \param config0 The register value to be set for settings 0.
 *
 * \param config1 The register value to be set for settings 1.
 *
 * \retval ::FWK_SUCCESS The operation succeeded.
 * \retval ::FWK_E_PARAM The `pll_select` parameter was an invalid value.
 */
int ccsm_drv_set_pll_dynamic_settings(
    struct ccsm_reg *reg,
    enum mod_ccsm_pll_dynamic_select pll_select,
    uint32_t config0,
    uint32_t config1);

/*!
 * \brief Get current droop mitigation config.
 *
 * \param reg Pointer to the CCSM register struct.
 *
 * \param[out] config Contents of current droop mitigation config register.
 *
 * \retval ::FWK_SUCCESS The operation succeeded.
 */
int ccsm_drv_get_dm_configuration(struct ccsm_reg *reg, uint32_t *config);

/*!
 * \brief Get droop mitigation telemetry. Requires debug to be enabled.
 *
 * \param reg Pointer to the CCSM register struct.
 *
 * \param[out] telemetry1 Contents of telemetry register 1.
 *
 * \param[out] telemetry2 Contents of telemetry register 2.
 *
 * \param[out] telemetry3 Contents of telemetry register 3.
 *
 * \retval ::FWK_SUCCESS The operation succeeded.
 */
int ccsm_drv_get_dm_telemetry(
    struct ccsm_reg *reg,
    uint32_t *telemetry1,
    uint32_t *telemetry2,
    uint32_t *telemetry3);

/*!
 * \brief Set target droop mitigation config, applied in next DVFS/DM command.
 *
 * \param reg Pointer to the CCSM register struct.
 *
 * \param[in] config The target droop mitigation config register value to set.
 *
 * \retval ::FWK_SUCCESS The operation succeeded.
 */
int ccsm_drv_set_dm_configuration(struct ccsm_reg *reg, uint32_t config);

/*!
 * \brief Get current modulator config for a core.
 *
 * \param reg Pointer to the CCSM register struct.
 *
 * \param core_id Identifier for the core.
 *
 * \param[out] config The current modulator config register value.
 *
 * \retval ::FWK_SUCCESS The operation succeeded.
 * \retval ::FWK_E_PARAM The `core_id` parameter was an invalid value.
 */
int ccsm_drv_get_mod_configuration(
    struct ccsm_reg *reg,
    enum mod_ccsm_mod_select core_id,
    uint32_t *config);

/*!
 * \brief Set modulator configuration, to be applied in next set_mod command.
 *
 * \param reg Pointer to the CCSM register struct.
 *
 * \param core_id Identifier for the core to be updated.
 *
 * \param config The target modulator config register value to set.
 *
 * \retval ::FWK_SUCCESS The operation succeeded.
 * \retval ::FWK_E_PARAM The `core_id` parameter was an invalid value.
 */
int ccsm_drv_set_mod_configuration(
    struct ccsm_reg *reg,
    enum mod_ccsm_mod_select core_id,
    uint32_t config);

/*!
 * \brief Check if IRQ error register is clear.
 *
 * \param reg Pointer to the CCSM register struct.
 *
 * \retval true The IRQ error register is clear.
 * \retval false The IRQ error register is not clear.
 */
bool ccsm_drv_is_irq_error_clear(void *reg);

/*!
 * \brief Get IRQ error register and clear.
 *
 * \param reg Pointer to the CCSM register struct.
 *
 * \param[out] error The IRQ error register contents.
 *
 * \retval ::FWK_SUCCESS The operation succeeded.
 */
int ccsm_drv_get_clear_irq_error(struct ccsm_reg *reg, uint32_t *error);

/*!
 * \brief Check if IRQ status register is clear.
 *
 * \param reg Pointer to the CCSM register struct.
 *
 * \retval true The IRQ status register is clear.
 * \retval false The IRQ status register is not clear.
 */
bool ccsm_drv_is_irq_status_clear(void *reg);

/*!
 * \brief Get IRQ status register and clear.
 *
 * \param reg Pointer to the CCSM register struct.
 *
 * \param[out] status The IRQ status register contents.
 *
 * \retval ::FWK_SUCCESS The operation succeeded.
 */
int ccsm_drv_get_clear_irq_status(struct ccsm_reg *reg, uint32_t *status);

/*!
 * \brief Check if current CCSM request is complete.
 *
 * \param reg Pointer to the CCSM register struct.
 *
 * \retval true The request is complete.
 * \retval false The request is ongoing.
 */
bool ccsm_drv_is_request_complete(void *reg);

/*!
 * \brief Set request registers and trigger a CCSM request.
 *
 * \param reg Pointer to the CCSM register struct.
 *
 * \param req_type Type of request to be triggered.
 *
 * \retval ::FWK_SUCCESS The operation succeeded.
 * \retval ::FWK_E_DEVICE A request has not been cleared from the device.
 * \retval ::FWK_E_BUSY A request is still ongoing.
 */
int ccsm_drv_set_request(
    struct ccsm_reg *reg,
    enum mod_ccsm_request_type req_type);

/*!
 * \brief Clear request registers.
 *
 * \param reg Pointer to the CCSM register struct.
 *
 * \retval ::FWK_SUCCESS The operation succeeded.
 */
int ccsm_drv_clear_request(struct ccsm_reg *reg);
