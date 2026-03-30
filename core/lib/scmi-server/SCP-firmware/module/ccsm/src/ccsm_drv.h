/*
 * Arm SCP/MCP Software
 * Copyright (c) 2024, Arm Limited and Contributors. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef CCSM_DRV_H
#define CCSM_DRV_H

#include <fwk_element.h>
#include <fwk_id.h>
#include <fwk_macros.h>

#include <stdbool.h>
#include <stdint.h>

/*! CCSM blocking timeout durations (microseconds). */
#define MOD_CCSM_CLEAR_IRQ_TIMEOUT    100
#define MOD_CCSM_COMMAND_WAIT_TIMEOUT 100

/*! CCSM Register Definitions */
/* clang-format off */
struct ccsm_reg {
    FWK_RW uint32_t PLL_NOM_ARCH;                       /* 0x00 */
    FWK_RW uint32_t PLL_NOMINAL_SETTINGS_0;             /* 0x04 */
    FWK_RW uint32_t PLL_NOMINAL_SETTINGS_1;             /* 0x08 */
           uint32_t RESERVED1;
    FWK_RW uint32_t PLL_FALLBACK_ARCH;                  /* 0x10 */
    FWK_RW uint32_t PLL_FALLBACK_SETTINGS_0;            /* 0x14 */
    FWK_RW uint32_t PLL_FALLBACK_SETTINGS_1;            /* 0x18 */
           uint32_t RESERVED2;
    FWK_RW uint32_t PLL_0_STATIC;                       /* 0x20 */
    FWK_RW uint32_t PLL_1_STATIC;                       /* 0x24 */
           uint32_t RESERVED3[2];
    FWK_RW uint32_t RAM_EMA_CFG_rf_2p1r1rwddata;        /* 0x30 */
    FWK_RW uint32_t RAM_EMA_CFG_rf_2p;                  /* 0x34 */
    FWK_RW uint32_t RAM_EMA_CFG_rf_sp;                  /* 0x38 */
    FWK_RW uint32_t RAM_EMA_CFG_sram_sp;                /* 0x3C */
    FWK_RW uint32_t RAM_EMA_CFG_sram_2p;                /* 0x40 */
           uint32_t RESERVED4[3];
    FWK_RW uint32_t PMIC_VOLTAGE_STABLE;                /* 0x50 */
           uint32_t RESERVED5[3];
    FWK_RW uint32_t DROOP_MITIGATION_STRATEGY;          /* 0x60 */
           uint32_t RESERVED6[3];
    FWK_RW uint32_t MODULATOR_SETTINGS_CORE0;           /* 0x70 */
    FWK_RW uint32_t MODULATOR_SETTINGS_CORE1;           /* 0x74 */
    FWK_RW uint32_t MODULATOR_SETTINGS_CORE2;           /* 0x78 */
    FWK_RW uint32_t MODULATOR_SETTINGS_CORE3;           /* 0x7C */
           uint32_t RESERVED7[4];
    FWK_RW uint32_t REQUEST_TYPE;                       /* 0x90 */
           uint32_t RESERVED8[3];
    FWK_W  uint32_t REQUEST_ENABLE;                     /* 0xA0 */
    FWK_RW uint32_t IRQ_STATUS_MASK;                    /* 0xA4 */
           uint32_t RESERVED9[3];
    FWK_W  uint32_t IRQ_STATUS_CLEAR;                   /* 0xB4 */
    FWK_W  uint32_t IRQ_ERROR_CLEAR;                    /* 0xB8 */
           uint32_t RESERVED10[17];
    FWK_R  uint32_t INT_PLL_NOM_ARCH;                   /* 0x100 */
    FWK_R  uint32_t INT_PLL_NOMINAL_SETTINGS_0;         /* 0x104 */
    FWK_R  uint32_t INT_PLL_NOMINAL_SETTINGS_1;         /* 0x108 */
           uint32_t RESERVED11;
    FWK_R  uint32_t INT_PLL_FALLBACK_ARCH;              /* 0x110 */
    FWK_R  uint32_t INT_PLL_FALLBACK_SETTINGS_0;        /* 0x114 */
    FWK_R  uint32_t INT_PLL_FALLBACK_SETTINGS_1;        /* 0x118 */
           uint32_t RESERVED12;
    FWK_R  uint32_t INT_PLL_0_STATIC;                   /* 0x120 */
    FWK_R  uint32_t INT_PLL_1_STATIC;                   /* 0x124 */
           uint32_t RESERVED13[2];
    FWK_R  uint32_t INT_RAM_EMA_CFG_rf_2p1r1rwddata;    /* 0x130 */
    FWK_R  uint32_t INT_RAM_EMA_CFG_rf_2p;              /* 0x134 */
    FWK_R  uint32_t INT_RAM_EMA_CFG_rf_sp;              /* 0x138 */
    FWK_R  uint32_t INT_RAM_EMA_CFG_sram_sp;            /* 0x13C */
    FWK_R  uint32_t INT_RAM_EMA_CFG_sram_2p;            /* 0x140 */
           uint32_t RESERVED14[3];
    FWK_R  uint32_t INT_PMIC_VOLTAGE_STABLE;            /* 0x150 */
           uint32_t RESERVED15[3];
    FWK_R  uint32_t INT_DROOP_MITIGATION_STRATEGY;      /* 0x160 */
           uint32_t RESERVED16[3];
    FWK_R  uint32_t INT_MODULATOR_SETTINGS_CORE0;       /* 0x170 */
    FWK_R  uint32_t INT_MODULATOR_SETTINGS_CORE1;       /* 0x174 */
    FWK_R  uint32_t INT_MODULATOR_SETTINGS_CORE2;       /* 0x178 */
    FWK_R  uint32_t INT_MODULATOR_SETTINGS_CORE3;       /* 0x17C */
           uint32_t RESERVED17[4];
    FWK_R  uint32_t INT_REQUEST_TYPE;                   /* 0x190 */
           uint32_t RESERVED18[3];
    FWK_R  uint32_t REQUEST_STATUS;                     /* 0x1A0 */
    FWK_R  uint32_t IRQ_STATUS;                         /* 0x1A4 */
    FWK_R  uint32_t IRQ_ERROR;                          /* 0x1A8 */
           uint32_t RESERVED19;
    FWK_R  uint32_t CONTROL_SM_STATE;                   /* 0x1B0 */
    FWK_R  uint32_t DVFS_SM_STATE;                      /* 0x1B4 */
    FWK_R  uint32_t DM_SM_STATE;                        /* 0x1B8 */
    FWK_R  uint32_t OC_SM_STATE;                        /* 0x1BC */
    FWK_R  uint32_t DROOP_MITIGATION_TELEMETRY1;        /* 0x1C0 */
    FWK_R  uint32_t DROOP_MITIGATION_TELEMETRY2;        /* 0x1C4 */
    FWK_R  uint32_t DROOP_MITIGATION_TELEMETRY3;        /* 0x1C8 */
};
/* clang-format on */

/*! Static configuration PLL selection. */
enum mod_ccsm_pll_static_select { MOD_CCSM_PLL_0, MOD_CCSM_PLL_1 };

/*! Dynamic configuration PLL selection. */
enum mod_ccsm_pll_dynamic_select {
    MOD_CCSM_PLL_NOMINAL,
    MOD_CCSM_PLL_FALLBACK
};

/*! Modulator configuration selection. */
enum mod_ccsm_mod_select {
    MOD_CCSM_MOD_CORE0,
    MOD_CCSM_MOD_CORE1,
    MOD_CCSM_MOD_CORE2,
    MOD_CCSM_MOD_CORE3,
    MOD_CCSM_MOD_CORE_MAXCOUNT
};

/*! Values for request type register. */
enum mod_ccsm_request_type {
    /*! Clear any requests from the register. */
    CCSM_REQUEST_TYPE_CLR = 0,
    /*! Request reconfiguration of DVFS state machine and registers. */
    CCSM_REQUEST_TYPE_SET_DVFS = 1 << 0,
    /*!
     * \brief Request first step of a DVFS GO_UP transition.
     *
     * \details Requests a DVFS transition to a higher voltage/frequency point.
     *      On successful completion a GO_UP2 request must be made.
     */
    CCSM_REQUEST_TYPE_GO_UP = 1 << 1,
    /*!
     * \brief Request first step of a DVFS GO_DN transition.
     *
     * \details Requests a DVFS transition to a lower voltage/frequency point.
     *      On successful completion a GO_DN2 request must be made.
     */
    CCSM_REQUEST_TYPE_GO_DN = 1 << 2,
    /*!
     * \brief Request second step of a DVFS GO_UP transition.
     *
     * \details Completes DVFS transition to a higher voltage/frequency point.
            This request will also update droop mitigation configuration, if
            changed.
     */
    CCSM_REQUEST_TYPE_GO_UP2 = 1 << 3,
    /*!
     * \brief Request second step of a DVFS GO_DN transition.
     *
     * \details Completes DVFS transition to a lower voltage/frequency point.
            This request will also update droop mitigation configuration, if
            changed.
     */
    CCSM_REQUEST_TYPE_GO_DN2 = 1 << 4,
    /*!
     * \brief Request reconfiguration of droop mitigation state machine.
     */
    CCSM_REQUEST_TYPE_SET_DM = 1 << 8,
    /*!
     * \brief Put overcurrent state machine into idle state.
     *
     * \details Restores the overcurrent state machine to idle following a OC
            PMIC warning. This will also clear the err_irq_oc register.
     */
    CCSM_REQUEST_TYPE_CLR_MOD = 1 << 16,
    /*! Request reconfiguration of modulator(s). */
    CCSM_REQUEST_TYPE_SET_MOD = 1 << 17,
    /*!
     * \brief Request telemetry information.
     *
     * \details Triggers a request to copy internal debug information to the
            telemetry registers. This contains details on the types of droop
            events that have occurrred since the last debug request.
     */
    CCSM_REQUEST_TYPE_DBG = 1 << 24
};

/*! Values for request enable register. */
enum mod_ccsm_request_enable {
    /*! Clear the register after a request. */
    CCSM_REQUEST_ENABLE_CLR = 0,
    /*! Trigger the request enabled in the request type register. */
    CCSM_REQUEST_ENABLE_SET = 1
};

/*! Values for request status register. */
enum mod_ccsm_request_status {
    /*! Ongoing bit. Set at beginning of task, cleared on completion. */
    CCSM_REQUEST_STATUS_OG = 1
};

/*! Value offsets in droop mitigation register. */
enum mod_ccsm_droop_mitigation_shift {
    CCSM_DROOP_MITIGATION_STRATEGY_TRANSITION_PAUSE_POS = 0,
    CCSM_DROOP_MITIGATION_STRATEGY_MITIGATION_DURATION_POS = 4
};

/*! Value masks for droop mitigation register. */
enum mod_ccsm_droop_mitigation_mask {
    CCSM_DROOP_MITIGATION_STRATEGY_TRANSITION_PAUSE_MASK = 0x0F,
    CCSM_DROOP_MITIGATION_STRATEGY_MITIGATION_DURATION_MASK = 0xFFF0
};

/*! Value offsets in modulator registers. */
enum mod_ccsm_modulator_shift {
    CCSM_MODULATOR_SETTINGS_NUMERATOR_REGULAR_POS = 0,
    CCSM_MODULATOR_SETTINGS_DENOMINATOR_POS = 8,
    CCSM_MODULATOR_SETTINGS_NUMERATOR_PMIC_OC_POS = 16
};

/*! Value masks for modulator registers. */
enum mod_ccsm_modulator_mask {
    CCSM_MODULATOR_SETTINGS_NUMERATOR_REGULAR_MASK = 0x0000FF,
    CCSM_MODULATOR_SETTINGS_DENOMINATOR_MASK = 0x00FF00,
    CCSM_MODULATOR_SETTINGS_NUMERATOR_PMIC_OC_MASK = 0xFF0000
};

/*! Status values for control state machine. */
enum mod_ccsm_control_sm_status_shift {
    /*! Current state of the state machine. */
    CCSM_CONTROL_SM_CURRENT_STATE = 1 << 0,
    /*! Previous state of the state machine. */
    CCSM_CONTROL_SM_PREV_STATE = 1 << 8,
    /*! State before the previous state of the state machine. */
    CCSM_CONTROL_SM_BEFORE_PREV_STATE = 1 << 16,
    /*! Lower bit of clksel_nom, selects PLL0 as output. */
    CCSM_CONTROL_SM_CLKSEL_NOM0 = 1 << 24,
    /*! Upper bit of clksel_nom, selects PLL1 as output. */
    CCSM_CONTROL_SM_CLKSEL_NOM1 = 1 << 25,
    /*! Lower bit of clksel_fb, selects PLL0 as output. */
    CCSM_CONTROL_SM_CLKSEL_FB0 = 1 << 26,
    /*! Upper bit of clksel_fb, selects PLL1 as output. */
    CCSM_CONTROL_SM_CLKSEL_FB1 = 1 << 27,
    /*! Current value of the PLL_SWAP signal. */
    CCSM_CONTROL_SM_PLL_SWAP = 1 << 28
};

/*! Status values for DVFS state machine. */
enum mod_ccsm_dvfs_sm_status_shift {
    /*! Current state of the state machine. */
    CCSM_DVFS_SM_CURRENT_STATE = 1 << 0,
    /*! Previous state of the state machine. */
    CCSM_DVFS_SM_PREV_STATE = 1 << 8,
    /*! State before the previous state of the state machine. */
    CCSM_DVFS_SM_BEFORE_PREV_STATE = 1 << 16,
    /*! Bit indicating if a second DVFS up step is expected. */
    CCSM_DVFS_GO_UP2_REQD = 1 << 24,
    /*! Bit indicating if a second DVFS down step is expected. */
    CCSM_DVFS_GO_DN2_REQD = 1 << 25
};

/*!
 * \brief Status values for droop mitigation state machine.
 */
enum mod_ccsm_dm_sm_status_shift {
    /*! Current state of the state machine. */
    CCSM_DM_SM_CURRENT_STATE = 1 << 0,
    /*! 2-bit value of CLKSEL, selecting nominal or fallback. */
    CCSM_DM_SM_CLKSEL = 1 << 8,
};

/*!
 * \brief Status values for overcurrent state machine.
 */
enum mod_ccsm_oc_sm_status_shift {
    /*! Current state of the state machine. */
    CCSM_OC_SM_CURRENT_STATE = 1 << 0
};

/*! CCSM driver functions */

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

#endif /* CCSM_DRV_H */
