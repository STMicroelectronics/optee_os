/*
 * Arm SCP/MCP Software
 * Copyright (c) 2024, Arm Limited and Contributors. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 * Description:
 *     SPMI HAL module - a module requiring access to peripheral through
 *     the SPMI bus can bind to this module
 */

#include <internal/spmi.h>

#include <mod_spmi.h>

#include <fwk_assert.h>
#include <fwk_core.h>
#include <fwk_event.h>
#include <fwk_id.h>
#include <fwk_log.h>
#include <fwk_mm.h>
#include <fwk_module.h>
#include <fwk_module_idx.h>
#include <fwk_status.h>

#include <stdbool.h>

/*! SPMI API identifier */
const fwk_id_t mod_spmi_api_id_spmi =
    FWK_ID_API_INIT(FWK_MODULE_IDX_SPMI, MOD_SPMI_API_IDX_SPMI);

/*! Driver response API identifier */
const fwk_id_t mod_spmi_api_id_driver_response =
    FWK_ID_API_INIT(FWK_MODULE_IDX_SPMI, MOD_SPMI_API_IDX_DRIVER_RESPONSE);

/*! State of the SPMI device - idle, busy or ready to process next request */
enum mod_spmi_dev_state {
    /*! The SPMI controller driver is not processing any request, and
        SPMI HAL has no request pending to be sent to SPMI driver */
    MOD_SPMI_DEV_STATE_IDLE,
    /*! The SPMI controller driver is processing a request */
    MOD_SPMI_DEV_STATE_BUSY,
    /*! The SPMI HAL is in a unrecoverable error state and cannot
        process any requests */
    MOD_SPMI_DEV_STATE_PANIC,
};

/*! Context for SPMI device */
struct mod_spmi_dev_ctx {
    const struct mod_spmi_dev_config *config;
    const struct mod_spmi_driver_api *driver_api;
    struct mod_spmi_request request;
    enum mod_spmi_dev_state state;
};

static struct mod_spmi_dev_ctx *ctx_table;

/*
 * Static helpers
 */
static inline void get_ctx(fwk_id_t id, struct mod_spmi_dev_ctx **ctx)
{
    *ctx = ctx_table + fwk_id_get_element_idx(id);
}

static int respond_to_caller(fwk_id_t dev_id, int drv_status)
{
    int status;
    struct fwk_event resp;
    struct mod_spmi_event_param *param =
        (struct mod_spmi_event_param *)resp.params;

    status = fwk_get_first_delayed_response(dev_id, &resp);
    if (status != FWK_SUCCESS) {
        return status;
    }

    param->status = drv_status;

    return fwk_put_event(&resp);
}

static inline int send_command_and_set_dev_state(
    struct mod_spmi_dev_ctx *ctx,
    struct mod_spmi_request *request,
    struct fwk_event *resp_event)
{
    int status;
    struct mod_spmi_event_param *resp_param;

    ctx->request = *request;
    ctx->state = MOD_SPMI_DEV_STATE_BUSY;
    status =
        ctx->driver_api->send_command(ctx->config->driver_id, &ctx->request);
    if (status == FWK_PENDING) {
        resp_event->is_delayed_response = true;
    } else {
        /* The request has succeeded or failed, respond now */
        resp_param = (struct mod_spmi_event_param *)resp_event->params;
        resp_param->status = status;
        ctx->state = MOD_SPMI_DEV_STATE_IDLE;
    }

    return status;
}

static int mod_spmi_process_send_command(
    struct mod_spmi_dev_ctx *ctx,
    const struct fwk_event *event,
    struct fwk_event *resp_event)
{
    int status = FWK_SUCCESS;
    struct mod_spmi_event_param *event_param;

    switch (ctx->state) {
    case MOD_SPMI_DEV_STATE_IDLE:
        status = send_command_and_set_dev_state(
            ctx, (struct mod_spmi_request *)event->params, resp_event);
        break;

    case MOD_SPMI_DEV_STATE_BUSY:
        FWK_LOG_DEBUG("SPMI HAL: SPMI device busy - queueing up request");
        resp_event->is_delayed_response = true;
        break;

    case MOD_SPMI_DEV_STATE_PANIC:
    default:
        FWK_LOG_ERR(
            "SPMI HAL: SPMI device in panic or unknown state"
            "- Cannot handle request");
        event_param = (struct mod_spmi_event_param *)resp_event->params;
        event_param->status = FWK_E_PANIC;
        return FWK_E_PANIC;
    }

    return status;
}

static int process_next_request(fwk_id_t dev_id, struct mod_spmi_dev_ctx *ctx)
{
    int status, drv_status;
    bool is_empty;
    struct fwk_event delayed_response;
    struct fwk_event_light next_event;

    status = fwk_is_delayed_response_list_empty(dev_id, &is_empty);
    if (status != FWK_SUCCESS) {
        return status;
    }

    if (is_empty) {
        ctx->state = MOD_SPMI_DEV_STATE_IDLE;
        return FWK_SUCCESS;
    }

    status = fwk_get_first_delayed_response(dev_id, &delayed_response);
    if (status != FWK_SUCCESS) {
        return status;
    }

    drv_status = send_command_and_set_dev_state(
        ctx,
        (struct mod_spmi_request *)delayed_response.params,
        &delayed_response);
    if (drv_status != FWK_PENDING) {
        /* Driver has responded synchronously, send the delayed response now*/
        status = fwk_put_event(&delayed_response);

        fwk_is_delayed_response_list_empty(dev_id, &is_empty);
        if (!is_empty) {
            next_event.target_id = dev_id;
            next_event.id = mod_spmi_event_id_process_next_request;
            status = fwk_put_event(&next_event);
        }
    }

    return status;
}

static int create_spmi_request(
    fwk_id_t dev_id,
    struct mod_spmi_request *request)
{
    int status;
    struct fwk_event event;
    struct mod_spmi_request *event_param =
        (struct mod_spmi_request *)event.params;

    event = (struct fwk_event){
        .target_id = dev_id,
        .response_requested = true,
        .id = mod_spmi_event_id_send_command,
    };
    *event_param = *request;

    status = fwk_put_event(&event);
    if (status == FWK_SUCCESS) {
        /*
         * The request has been successfully queued for later processing by the
         * SPMI device but processing of this request has not yet begun. The
         * caller is notified that the SPMI request is in progress.
         */
        return FWK_PENDING;
    }

    return status;
}

/*
 * SPMI API
 */
static int mod_spmi_send_completer_read_command(
    fwk_id_t dev_id,
    uint8_t target_id,
    uint32_t reg_addr,
    uint8_t *data,
    uint8_t byte_count)
{
    if (!fwk_expect(byte_count != 0)) {
        return FWK_E_PARAM;
    }

    if (!fwk_expect(data != NULL)) {
        return FWK_E_PARAM;
    }

    struct mod_spmi_request request = {
        .target_id = target_id,
        .reg_address = reg_addr,
        .data = data,
        .byte_count = byte_count,
        .command = MOD_SPMI_CMD_REG_READ,
    };

    if (reg_addr & ~(SPMI_8_BIT_ADDRESS_MASK)) {
        request.command = MOD_SPMI_CMD_EXT_REG_READ_LONG;
    } else if (reg_addr & ~(SPMI_5_BIT_ADDRESS_MASK)) {
        request.command = MOD_SPMI_CMD_EXT_REG_READ;
    }

    return create_spmi_request(dev_id, &request);
}

static int mod_spmi_send_completer_write_command(
    fwk_id_t dev_id,
    uint8_t target_id,
    uint32_t reg_addr,
    uint8_t *data,
    uint8_t byte_count)
{
    if (!fwk_expect(byte_count != 0)) {
        return FWK_E_PARAM;
    }

    if (!fwk_expect(data != NULL)) {
        return FWK_E_PARAM;
    }

    struct mod_spmi_request request = {
        .target_id = target_id,
        .reg_address = reg_addr,
        .data = data,
        .byte_count = byte_count,
        .command = MOD_SPMI_CMD_REG_WRITE,
    };

    if (reg_addr & ~(SPMI_8_BIT_ADDRESS_MASK)) {
        request.command = MOD_SPMI_CMD_EXT_REG_WRITE_LONG;
    } else if (reg_addr & ~(SPMI_5_BIT_ADDRESS_MASK)) {
        request.command = MOD_SPMI_CMD_EXT_REG_WRITE;
    } else if (reg_addr == SPMI_REG_0_ADDRESS) {
        request.command = MOD_SPMI_CMD_REG0_WRITE;
    }

    return create_spmi_request(dev_id, &request);
}

static int mod_spmi_send_power_command(
    fwk_id_t dev_id,
    uint8_t target_id,
    enum mod_spmi_command command)
{
    /* Validate the command */
    if ((command < MOD_SPMI_CMD_RESET) || (command > MOD_SPMI_CMD_WAKEUP)) {
        FWK_LOG_ERR("SPMI HAL: invalid power command");
        return FWK_E_PARAM;
    }

    struct mod_spmi_request request = {
        .target_id = target_id,
        .command = command,
    };

    return create_spmi_request(dev_id, &request);
}

static int mod_spmi_send_auth_command(
    fwk_id_t dev_id,
    uint8_t target_id,
    uint8_t *data,
    uint8_t byte_count)
{
    struct mod_spmi_request request = {
        .target_id = target_id,
        .command = MOD_SPMI_CMD_AUTHENTICATE,
        .data = data,
        .byte_count = byte_count,
    };

    return create_spmi_request(dev_id, &request);
}

static int mod_spmi_send_requester_read(
    fwk_id_t dev_id,
    uint8_t target_id,
    uint32_t reg_addr,
    uint8_t *data)
{
    struct mod_spmi_request request = {
        .target_id = target_id,
        .command = MOD_SPMI_CMD_MASTER_READ,
        .data = data,
    };

    return create_spmi_request(dev_id, &request);
}

static int mod_spmi_send_requester_write(
    fwk_id_t dev_id,
    uint8_t target_id,
    uint32_t reg_addr,
    uint8_t *data)
{
    struct mod_spmi_request request = {
        .target_id = target_id,
        .command = MOD_SPMI_CMD_MASTER_WRITE,
        .data = data,
    };

    return create_spmi_request(dev_id, &request);
}

static int mod_spmi_send_ddb_read(
    fwk_id_t dev_id,
    uint8_t target_id,
    bool is_requester,
    uint8_t *data)
{
    struct mod_spmi_request request = {
        .target_id = target_id,
        .command = MOD_SPMI_CMD_DDB_MASTER_READ,
        .data = data,
    };

    return create_spmi_request(dev_id, &request);
}

static struct mod_spmi_api spmi_api = {
    .completer_read = mod_spmi_send_completer_read_command,
    .completer_write = mod_spmi_send_completer_write_command,
    .power_operation = mod_spmi_send_power_command,
    .authenticate = mod_spmi_send_auth_command,
    .requester_read = mod_spmi_send_requester_read,
    .requester_write = mod_spmi_send_requester_write,
    .device_descriptor_block_read = mod_spmi_send_ddb_read
};

/*
 * Driver response API
 */
static void transaction_completed(fwk_id_t dev_id, int spmi_status)
{
    int status;
    struct fwk_event event;
    struct mod_spmi_event_param *param =
        (struct mod_spmi_event_param *)event.params;

    event = (struct fwk_event){
        .target_id = dev_id,
        .source_id = dev_id,
        .id = mod_spmi_event_id_transaction_completed,
    };

    param->status = spmi_status;

    status = fwk_put_event(&event);
    fwk_assert(status == FWK_SUCCESS);
}

static struct mod_spmi_driver_response_api driver_response_api = {
    .transaction_completed = transaction_completed,
};

/*
 * Framework handlers
 */
static int mod_spmi_init(
    fwk_id_t module_id,
    unsigned int element_count,
    const void *unused)
{
    ctx_table = fwk_mm_calloc(element_count, sizeof(ctx_table[0]));

    return FWK_SUCCESS;
}

static int mod_spmi_dev_init(
    fwk_id_t element_id,
    unsigned int unused,
    const void *data)
{
    struct mod_spmi_dev_ctx *ctx;

    get_ctx(element_id, &ctx);
    ctx->config = (struct mod_spmi_dev_config *)data;
    ctx->state = MOD_SPMI_DEV_STATE_IDLE;

    return FWK_SUCCESS;
}

static int mod_spmi_bind(fwk_id_t id, unsigned int round)
{
    int status;
    struct mod_spmi_dev_ctx *ctx;

    /*
     * Only bind in first round of calls
     * Nothing to do for module
     */
    if ((round > 0) || fwk_id_is_type(id, FWK_ID_TYPE_MODULE)) {
        return FWK_SUCCESS;
    }

    get_ctx(id, &ctx);

    /* Bind to driver */
    status = fwk_module_bind(
        ctx->config->driver_id, ctx->config->api_id, &ctx->driver_api);

    return status;
}

static int mod_spmi_process_bind_request(
    fwk_id_t source_id,
    fwk_id_t target_id,
    fwk_id_t api_id,
    const void **api)
{
    struct mod_spmi_dev_ctx *ctx;

    if (!fwk_id_is_type(target_id, FWK_ID_TYPE_ELEMENT)) {
        return FWK_E_PARAM;
    }

    get_ctx(target_id, &ctx);

    switch (fwk_id_get_api_idx(api_id)) {
    case MOD_SPMI_API_IDX_DRIVER_RESPONSE:
        if (fwk_id_is_equal(source_id, ctx->config->driver_id)) {
            *api = &driver_response_api;
        } else {
            return FWK_E_PARAM;
        }
        break;

    case MOD_SPMI_API_IDX_SPMI:
        *api = &spmi_api;
        break;

    default:
        return FWK_E_PARAM;
    }

    return FWK_SUCCESS;
}

static int mod_spmi_process_event(
    const struct fwk_event *event,
    struct fwk_event *resp_event)
{
    int status;
    struct mod_spmi_dev_ctx *ctx;
    struct mod_spmi_event_param *event_param;
    enum mod_spmi_internal_event_idx event_id_type;

    get_ctx(event->target_id, &ctx);
    event_id_type =
        (enum mod_spmi_internal_event_idx)fwk_id_get_event_idx(event->id);

    switch (event_id_type) {
    case MOD_SPMI_EVENT_IDX_SEND_COMMAND:
        status = mod_spmi_process_send_command(ctx, event, resp_event);
        break;

    case MOD_SPMI_EVENT_IDX_TRANSACTION_COMPLETED:
        if (ctx->state != MOD_SPMI_DEV_STATE_BUSY) {
            return FWK_E_STATE;
        }

        event_param = (struct mod_spmi_event_param *)event->params;
        status = respond_to_caller(event->target_id, event_param->status);
        if (process_next_request(event->target_id, ctx) != FWK_SUCCESS) {
            return FWK_E_DEVICE;
        }
        break;

    case MOD_SPMI_EVENT_IDX_PROCESS_NEXT_REQUEST:
        status = process_next_request(event->target_id, ctx);
        break;

    default:
        FWK_LOG_ERR(
            "SPMI HAL: Received event of unknown type -- entering panic state");
        status = FWK_E_PANIC;
        break;
    }

    if (status != FWK_SUCCESS) {
        ctx->state = MOD_SPMI_DEV_STATE_PANIC;
    }

    return status;
}

const struct fwk_module module_spmi = {
    .type = FWK_MODULE_TYPE_HAL,
    .api_count = (unsigned int)MOD_SPMI_API_IDX_COUNT,
    .event_count = (unsigned int)MOD_SPMI_EVENT_IDX_TOTAL_COUNT,
    .init = mod_spmi_init,
    .element_init = mod_spmi_dev_init,
    .bind = mod_spmi_bind,
    .process_bind_request = mod_spmi_process_bind_request,
    .process_event = mod_spmi_process_event,
};
