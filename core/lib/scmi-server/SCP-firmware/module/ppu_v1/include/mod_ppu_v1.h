/*
 * Arm SCP/MCP Software
 * Copyright (c) 2015-2025, Arm Limited and Contributors. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 * Description:
 *     PPU v1 Driver
 */

#ifndef MOD_PPU_V1_H
#define MOD_PPU_V1_H

#include <mod_power_domain.h>

#include <fwk_id.h>

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

/*!
 * \addtogroup GroupModules Modules
 * \{
 */

/*!
 * \defgroup GroupModulePPUv1 PPUv1 Driver
 * \{
 */

/*!
 * \brief Default value if the value in the configuration is not set.
 */
#define DEFAULT_NUM_OF_CORES_IN_CLUSTER 8

/*!
 * \brief Indexes of the interfaces exposed by the module.
 */
enum mod_ppu_v1_api_idx {
    /*! Power domain driver API */
    MOD_PPU_V1_API_IDX_POWER_DOMAIN_DRIVER,
    /*! interrupt Service Routine driver API */
    MOD_PPU_V1_API_IDX_ISR,
    /*! System boot API */
    MOD_PPU_V1_API_IDX_BOOT,
    /*! Operating mode control API */
    MOD_PPU_V1_API_IDX_OPMODE_CTRL,
    /*! Number of exposed interfaces */
    MOD_PPU_V1_API_IDX_COUNT,
};

/*!
 * \brief PPU_V1 OPERATING MODES
 */
enum ppu_v1_opmode {
    PPU_V1_OPMODE_00,
    PPU_V1_OPMODE_01,
    PPU_V1_OPMODE_02,
    PPU_V1_OPMODE_03,
    PPU_V1_OPMODE_04,
    PPU_V1_OPMODE_05,
    PPU_V1_OPMODE_06,
    PPU_V1_OPMODE_07,
    PPU_V1_OPMODE_08,
    PPU_V1_OPMODE_09,
    PPU_V1_OPMODE_10,
    PPU_V1_OPMODE_11,
    PPU_V1_OPMODE_12,
    PPU_V1_OPMODE_13,
    PPU_V1_OPMODE_14,
    PPU_V1_OPMODE_15,
    /* No valid operating modes after this line */
    PPU_V1_OPMODE_COUNT
};

/*!
 * \brief Power domain PPU descriptor.
 */
struct mod_ppu_v1 {
    /*! Base address of the PPU registers */
    uintptr_t reg_base;

    /*! PPU's IRQ number */
    unsigned int irq;
};
/*!
 * \brief Timer for set_state.
 *
 * \details This structure is required to be filled in PPUv1 config file only
 *          when the timeout feature is required.
 */
struct mod_ppu_v1_timer_config {
    /*!
     * \brief Timer identifier.
     *
     * \details Used for binding with the timer API and waiting for specified
     *          delay after setting the PPU state.
     */
    fwk_id_t timer_id;

    /*!
     * PPU state change wait delay in micro seconds.
     * A valid non-zero value has to be specified when using this feature.
     */
    uint32_t set_state_timeout_us;
};

/*!
 * \brief PPU_V1 module configuration
 */
struct mod_ppu_v1_config {
    /*! Identifier of the power domain notification to register elements for */
    const fwk_id_t pd_notification_id;

    /*!
     * Identifier of the source module or element that is expected to send power
     * domain notifications.
     */
    fwk_id_t pd_source_id;

    /*! Number of cores in this cluster */
    uint32_t num_of_cores_in_cluster;

    /*! Set to true if the PPU is configured to operate in dynamic mode. */
    bool is_cluster_ppu_dynamic_mode_configured;
};

/*!
 * \brief Configuration data of a power domain of the PPU_V1 driver module.
 */
struct mod_ppu_v1_pd_config {
    /*! Power domain type */
    enum mod_pd_type pd_type;

    /*! PPU descriptor */
    struct mod_ppu_v1 ppu;

#ifdef BUILD_HAS_AE_EXTENSION
    /*! Base address of the Cluster AE registers */
    uintptr_t cluster_ae_reg_base;
#endif

    /*!
     *  In the case of a core power domain, identifier of the cluster power
     *  domain it belongs to. If the power domain is not a core power domain,
     *  the value of this field is undefined.
     */
    fwk_id_t cluster_id;

    /*!
     * Flag indicating if this domain should be powered on during element
     * init. This flag is only supported for device and system PPUs and should
     * not be set for any other type. Timeout is not provided at this stage.
     */
    bool default_power_on;

    /*!
     * \brief Identifier of an entity wishing to be notified when the PPU
     *     transitions out of the OFF state.
     *
     * \note This field may be set to ::FWK_ID_NONE, in which case no
     *     observer will be set.
     */
    fwk_id_t observer_id;

    /*!
     * \brief Identifier of the power state observer API implemented by
     *     ::mod_ppu_v1_pd_config::observer_id.
     */
    fwk_id_t observer_api;

    /*!
     * \brief Parameter passed to
     *     ::mod_ppu_v1_power_state_observer_api::post_ppu_on().
     */
    void *post_ppu_on_param;

    /*! Timer descriptor */
    struct mod_ppu_v1_timer_config *timer_config;

    /*!
     * \brief Identifier of the alarm for deeper locking timeout.
     *
     * \note This alarm is optional, if it is not defined it will not be
     * included.
     */
    fwk_optional_id_t alarm_id;

    /*!
     * Alarm delay period for the timeout. This is option; set to a
     * value greater than 0 if using the alarm.
     */
    uint32_t alarm_delay;

    /*! Enable operating mode support. */
    bool enable_opmode_support;

    /*! Enable dynamic operating mode policy.*/
    bool enable_opmode_dynamic_policy;

    /*! Defined primary operating mode to apply when an element is on. */
    enum ppu_v1_opmode min_op_mode;

    /*!
     * Product specific ppu opmode kept for backwards comptibility used when op
     * mode support is not enabled.
     */
    enum ppu_v1_opmode opmode;

    /*! Use optional op-mode interrupts (policy-complete / unsupported). */
    bool use_opmode_irqs;

    /*! Time out value for op mode updating if not using ISR not timer ctx */
    uint32_t opmode_time_out;
};

/*!
 * \brief PPU_V1 Power State Observer API.
 *
 * \details This API should be implemented by any modules that should be
 *     notified when a PPU changes state.
 */
struct mod_ppu_v1_power_state_observer_api {
    /*!
     * \brief Called after a PPU has turned on.
     *
     * \param param Generic configurable parameter.
     */
    void (*post_ppu_on)(void *param);
};

/*!
 * \brief PPU_V1 module ISR API
 */
struct ppu_v1_isr_api {
    /*!
     * \brief Handle a power domain PPU interrupt
     *
     * \param pd_id Identifier of the power domain
     */
    void (*ppu_interrupt_handler)(fwk_id_t pd_id);
};

/*!
 * \brief PPU_V1 module boot API
 */
struct ppu_v1_boot_api {
    /*!
     * \brief Power on a specified power domain
     *
     * \param pd_id Identifier of the power domain
     *
     * \retval ::FWK_SUCCESS Operation successful.
     * \return One of the standard framework error codes.
     */
    int (*power_mode_on)(fwk_id_t pd_id);
};
/*!
 * \brief PPUv1 operating-mode control API.
 */
struct mod_ppu_v1_opmode_ctrl_api {
    /*!
     * \brief Enable or disable operating-mode management for a power domain.
     *
     * \param pd_id   Power-domain element identifier.
     * \param enable  \c true to enable operating-mode management,
     *                \c false to disable it.
     *
     * \details Enabling does not require the power domain to be ON.  If the
     * domain is already ON when enabling, the current default operating mode
     * is applied immediately.
     *
     * \retval ::FWK_SUCCESS Operation completed successfully.
     */
    int (*set_enabled)(fwk_id_t pd_id, bool enable);

    /*!
     * \brief Enable or disable min **dynamic** operating-mode policy
     * (OP\_DYN\_EN).
     *
     * \param pd_id         Power-domain element identifier.
     * \param enable        \c true to enable dynamic OP policy,
     *                      \c false to disable it.
     * \param min_dyn_mode  Minimum operating mode allowed when the dynamic
     *                      policy is enabled.
     *
     * \note Enabling requires that operating-mode management is enabled and
     * the power domain is currently ON.
     *
     * \retval ::FWK_SUCCESS Operation completed successfully.
     * \retval ::FWK_E_STATE  The power domain is not ON, or OP-mode support
     *                         is disabled.
     */
    int (*enable_dynamic_policy)(
        fwk_id_t pd_id,
        bool enable,
        enum ppu_v1_opmode min_dyn_mode);

    /*!
     * \brief Set the minimum operating mode for the power domain.
     *
     * \param pd_id Power-domain element identifier.
     * \param opm   Operating mode to use as the minimum when the PD turns ON.
     *
     * \details If operating-mode management is enabled and the domain is
     * already ON, the new minimum is applied immediately (via IRQ or a timed
     * wait depending on configuration).
     *
     * \retval ::FWK_SUCCESS Operation completed or no change required.
     * \return Standard framework error code from the underlying request/set
     * path.
     */
    int (*set_min_opmode)(fwk_id_t pd_id, enum ppu_v1_opmode opm);

    /*!
     * \brief Request an immediate, one-shot operating-mode change.
     *
     * \param pd_id Power-domain element identifier.
     * \param opm   Operating mode to request now.
     *
     * \retval ::FWK_SUCCESS Operation completed (request queued or finished).
     * \retval ::FWK_E_SUPPORT OP-mode management is disabled for this PD.
     * \retval ::FWK_E_STATE   The power domain is not ON.
     * \return Standard framework error code from the underlying request/set
     * path.
     */
    int (*request_now)(fwk_id_t pd_id, enum ppu_v1_opmode opm);
};

/*!
 * \}
 */

/*!
 * \}
 */

#endif /* MOD_PPU_V1_H */
