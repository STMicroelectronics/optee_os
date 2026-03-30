/*
 * Arm SCP/MCP Software
 * Copyright (c) 2024, Arm Limited and Contributors. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 */

#include <mod_pid_controller.h>

#include <fwk_id.h>
#include <fwk_log.h>
#include <fwk_macros.h>
#include <fwk_mm.h>
#include <fwk_module.h>
#include <fwk_module_idx.h>
#include <fwk_status.h>

#define MOD_NAME "[PID_CTRL] "

/* Module API identifier */
const fwk_id_t mod_pid_controller_api_id_control = FWK_ID_API_INIT(
    FWK_MODULE_IDX_PID_CONTROLLER,
    MOD_PID_CONTROLLER_API_IDX_CONTROL);

/* Element Context */
struct mod_pid_controller_elem_ctx {
    /* Pointer to configuration data */
    const struct mod_pid_controller_elem_config *config;

    struct {
        /* Integral (accumulated) error */
        int64_t integral;

        /* Derivative error */
        int64_t derivative;

        /* Error from previous iteration */
        int64_t previous;

        /* Current error */
        int64_t current;
    } error;

    /* Control value */
    int64_t set_point;
};

/* Module Context */
struct mod_pid_contorller_ctx {
    /* Table of element context */
    struct mod_pid_controller_elem_ctx *elem_ctx_table;

    /* Number of elements */
    unsigned int element_count;
};

static struct mod_pid_contorller_ctx mod_ctx;

/*
 * Helper functions.
 */

static struct mod_pid_controller_elem_ctx *get_elem_ctx(fwk_id_t elem_id)
{
    unsigned int idx;

    idx = fwk_id_get_element_idx(elem_id);
    if (idx >= mod_ctx.element_count) {
        return NULL;
    }

    return &mod_ctx.elem_ctx_table[idx];
}

static inline void reset_error_values(
    struct mod_pid_controller_elem_ctx *pid_ctx)
{
    memset(&pid_ctx->error, 0, sizeof(pid_ctx->error));
}

static inline bool is_integral_error_overflow(
    struct mod_pid_controller_elem_ctx *pid_ctx)
{
    int64_t current = pid_ctx->error.current;
    int64_t integral = pid_ctx->error.integral;

    return (current > 0 && integral > (INT64_MAX - current)) ||
        (current < 0 && integral < (INT64_MIN - current));
}

static void integrate_error_with_anti_windup(
    struct mod_pid_controller_elem_ctx *pid_ctx)
{
    int64_t accumulated_error =
        pid_ctx->error.integral + pid_ctx->error.current;

    if ((pid_ctx->error.current < pid_ctx->config->integral_cutoff) &&
        (accumulated_error < pid_ctx->config->integral_max)) {
        /*
         * The error is below the cutoff value and,
         * the accumulated error is still within the maximum permissible
         * value, thus continue integration.
         */
        pid_ctx->error.integral = accumulated_error;
    }
}

static void calculate_derivate_error(
    struct mod_pid_controller_elem_ctx *pid_ctx)
{
    pid_ctx->error.derivative =
        pid_ctx->error.current - pid_ctx->error.previous;
    pid_ctx->error.previous = pid_ctx->error.current;
}

static inline const struct mod_pid_controller_k_parameter *get_k_prop(
    struct mod_pid_controller_elem_ctx *pid_ctx)
{
    if (pid_ctx->error.current < 0) {
        return &pid_ctx->config->k.proportional_overshoot;
    }

    return &pid_ctx->config->k.proportional_undershoot;
}

static inline int64_t compute_term(
    int64_t error,
    const struct mod_pid_controller_k_parameter *k)
{
    return (error * (int64_t)k->numerator) >> k->divisor_shift;
}

static int64_t calculate_output(struct mod_pid_controller_elem_ctx *pid_ctx)
{
    int64_t pid_output;

    pid_output = compute_term(pid_ctx->error.current, get_k_prop(pid_ctx)) +
        compute_term(pid_ctx->error.integral, &pid_ctx->config->k.integral) +
        compute_term(pid_ctx->error.derivative, &pid_ctx->config->k.derivative);

    /* Constrain output  to boundaries */
    pid_output = FWK_MAX(pid_output, pid_ctx->config->output.min);
    pid_output = FWK_MIN(pid_output, pid_ctx->config->output.max);

    return pid_output;
}

/*
 * API functions.
 */

static int pid_controller_update(fwk_id_t id, int64_t input, int64_t *output)
{
    struct mod_pid_controller_elem_ctx *pid_ctx;

    fwk_assert(output != NULL);

    pid_ctx = get_elem_ctx(id);
    fwk_assert(pid_ctx != NULL);

    if (input < pid_ctx->config->switch_on_value) {
        reset_error_values(pid_ctx);
        *output = pid_ctx->config->inactive_state_output;
        return FWK_SUCCESS;
    }

    pid_ctx->error.current = pid_ctx->set_point - input;
    if (is_integral_error_overflow(pid_ctx)) {
        FWK_LOG_ERR(MOD_NAME "Integral error overflow");
        return FWK_E_DATA;
    }

    /* Calculate the integral term */
    integrate_error_with_anti_windup(pid_ctx);

    /* Calculate the derivative term */
    calculate_derivate_error(pid_ctx);

    *output = calculate_output(pid_ctx);

    return FWK_SUCCESS;
}

static int pid_controller_set_point(fwk_id_t id, int64_t input)
{
    struct mod_pid_controller_elem_ctx *pid_ctx;

    pid_ctx = get_elem_ctx(id);
    fwk_assert(pid_ctx != NULL);

    if (input < pid_ctx->config->switch_on_value) {
        FWK_LOG_ERR(MOD_NAME "Error set control value under switch on value.");
        return FWK_E_PARAM;
    }

    pid_ctx->set_point = input;
    reset_error_values(pid_ctx);

    return FWK_SUCCESS;
}

static int pid_controller_reset(fwk_id_t id)
{
    struct mod_pid_controller_elem_ctx *pid_ctx;

    pid_ctx = get_elem_ctx(id);
    fwk_assert(pid_ctx != NULL);

    reset_error_values(pid_ctx);

    return FWK_SUCCESS;
}

const struct mod_pid_controller_api pid_controller_api = {
    .update = pid_controller_update,
    .set_point = pid_controller_set_point,
    .reset = pid_controller_reset,
};

/*
 * Framework handler functions.
 */

static int pid_controller_init(
    fwk_id_t module_id,
    unsigned int element_count,
    const void *data)
{
    mod_ctx.elem_ctx_table = fwk_mm_calloc(
        element_count, sizeof(struct mod_pid_controller_elem_ctx));
    mod_ctx.element_count = element_count;

    return FWK_SUCCESS;
}

static int pid_controller_element_init(
    fwk_id_t element_id,
    unsigned int unused,
    const void *data)
{
    int status;
    struct mod_pid_controller_elem_ctx *elem_ctx;
    const struct mod_pid_controller_elem_config *config =
        (const struct mod_pid_controller_elem_config *)data;

    elem_ctx = get_elem_ctx(element_id);
    fwk_assert(elem_ctx != NULL);
    elem_ctx->config = config;
    status = pid_controller_set_point(element_id, config->set_point);

    return status;
}

int pid_controller_bind_request(
    fwk_id_t source_id,
    fwk_id_t target_id,
    fwk_id_t api_id,
    const void **api)
{
    enum mod_pid_controller_api_idx api_idx;
    int status;

    api_idx = (enum mod_pid_controller_api_idx)fwk_id_get_api_idx(api_id);
    switch (api_idx) {
    case MOD_PID_CONTROLLER_API_IDX_CONTROL:
        *api = &pid_controller_api;
        status = FWK_SUCCESS;
        break;

    default:
        status = FWK_E_PARAM;
        break;
    }

    return status;
}

const struct fwk_module module_pid_controller = {
    .type = FWK_MODULE_TYPE_SERVICE,
    .event_count = (unsigned int)0,
    .api_count = (unsigned int)MOD_PID_CONTROLLER_API_IDX_COUNT,
    .init = pid_controller_init,
    .element_init = pid_controller_element_init,
    .process_bind_request = pid_controller_bind_request,
};
