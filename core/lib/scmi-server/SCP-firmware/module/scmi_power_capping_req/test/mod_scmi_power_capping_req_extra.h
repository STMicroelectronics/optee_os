/*
 * Arm SCP/MCP Software
 * Copyright (c) 2025, Arm Limited and Contributors. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 * Description:
 *      Power Capping Requester unit test support.
 */

#include <mod_scmi_power_capping_req.h>
#include <mod_timer.h>

int scmi_send_message(
    uint8_t scmi_message_id,
    uint8_t scmi_protocol_id,
    uint8_t token,
    fwk_id_t service_id,
    const void *payload,
    size_t payload_size,
    bool request_ack_by_interrupt);

int response_message_handler(fwk_id_t service_id);

int start_alarm_api(
    fwk_id_t alarm_id,
    uint32_t microseconds,
    enum mod_timer_alarm_type type,
    void (*callback)(uintptr_t param),
    uintptr_t param);

int stop_alarm_api(fwk_id_t alarm_id);

int fake_message_handler(
    fwk_id_t service_id,
    const void *payload,
    size_t payload_size);
