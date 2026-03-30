/*
 * Arm SCP/MCP Software
 * Copyright (c) 2024, Arm Limited and Contributors. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 * Description:
 *    Macros and functions used internally by SPMI HAL module
 */

#ifndef INTERNAL_SPMI_H
#define INTERNAL_SPMI_H

#include <mod_spmi.h>

/*! SPMI Register Address bitmasks */
#define SPMI_5_BIT_ADDRESS_MASK  0x0000001F
#define SPMI_8_BIT_ADDRESS_MASK  0x000000FF
#define SPMI_16_BIT_ADDRESS_MASK 0x0000FFFF

#define SPMI_REG_0_ADDRESS 0x00000000

/*!
 * \brief Event indices
 */
enum mod_spmi_internal_event_idx {
    MOD_SPMI_EVENT_IDX_SEND_COMMAND,
    MOD_SPMI_EVENT_IDX_TRANSACTION_COMPLETED,
    MOD_SPMI_EVENT_IDX_PROCESS_NEXT_REQUEST,
    MOD_SPMI_EVENT_IDX_TOTAL_COUNT,
};

/*!
 * \brief SPMI Power Commands
 */
enum mod_spmi_internal_power_command {
    MOD_SPMI_POWER_CMD_RESET = MOD_SPMI_CMD_RESET,
    MOD_SPMI_POWER_CMD_SLEEP,
    MOD_SPMI_POWER_CMD_SHUTDOWN,
    MOD_SPMI_POWER_CMD_WAKEUP,
};

/*! Send command to driver event identifier */
static const fwk_id_t mod_spmi_event_id_send_command =
    FWK_ID_EVENT_INIT(FWK_MODULE_IDX_SPMI, MOD_SPMI_EVENT_IDX_SEND_COMMAND);

/*! Request completed event identifier */
static const fwk_id_t mod_spmi_event_id_transaction_completed =
    FWK_ID_EVENT_INIT(
        FWK_MODULE_IDX_SPMI,
        MOD_SPMI_EVENT_IDX_TRANSACTION_COMPLETED);

/*! Process next request event identifier */
static const fwk_id_t mod_spmi_event_id_process_next_request =
    FWK_ID_EVENT_INIT(
        FWK_MODULE_IDX_SPMI,
        MOD_SPMI_EVENT_IDX_PROCESS_NEXT_REQUEST);

#endif /* INTERNAL_SPMI_H */
