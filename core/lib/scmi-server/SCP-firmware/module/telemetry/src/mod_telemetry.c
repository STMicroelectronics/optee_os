/*
 * Arm SCP/MCP Software
 * Copyright (c) 2025, Arm Limited and Contributors. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 * Description:
 *     Telemetry domain HAL (Hardware Abstraction Layer).
 *
 *     This module provides telemetry functionalities, including memory
 *     management for SHMTI (Shared Memory Telemetry Interface), bitmaps for
 *     allocation tracking, and functions for telemetry source and event
 *     management.
 */

#include <internal/shmti_macros.h>
#include <internal/telemetry.h>

#include <fwk_log.h>
#include <fwk_mm.h>
#include <fwk_module.h>
#include <fwk_module_idx.h>
#include <fwk_string.h>

#define MOD_NAME "[telemetry]"

#define CONSTRUCT_DE_HANDLE(handle, src_idx, grp_idx, de_idx) \
    do { \
        (handle).valid = 1; \
        (handle).source_index = (uint8_t)(src_idx); \
        (handle).group_index = (uint8_t)(grp_idx); \
        (handle).de_offset_index = (uint8_t)(de_idx); \
    } while (0)

#define VALIDATE_DE_HANDLE(handle)  ((bool)(handle.valid))
#define GET_SOURCE_INDEX(handle)    (handle.source_index)
#define GET_GROUP_INDEX(handle)     (handle.group_index)
#define GET_SOURCE_DE_INDEX(handle) (handle.de_offset_index)

/* Global telemetry context instance */
static struct mod_telemetry_context telemetry_ctx;
/* Forward declaration for telemetry update function */
static int start_telemetry_update(uint32_t sampling_rate_ms);
static int telemetry_disable(void);

static int telemetry_shmti_init(
    const struct mod_telemetry_shmti_info *shmti_info,
    uint32_t shmti_count)
{
    int status;
    struct telemetry_shmti_context *shmti_ctx_list;
    unsigned int index;

    /* Allocate memory for SHMTI context structures */
    shmti_ctx_list =
        fwk_mm_alloc(shmti_count, sizeof(struct telemetry_shmti_context));
    if (shmti_ctx_list == NULL) {
        return FWK_E_NOMEM;
    }

    /* Initialize each SHMTI context */
    for (index = 0; index < shmti_count; index++) {
        shmti_ctx_list[index].shmti_info = &shmti_info[index];
        status = shmti_create(&shmti_ctx_list[index]);
        if (status != FWK_SUCCESS) {
            return FWK_E_PANIC;
        }
    }

    telemetry_ctx.shmti_count = shmti_count;
    telemetry_ctx.shmti_ctx_table = shmti_ctx_list;

    return FWK_SUCCESS;
}

static int telemetry_get_num_de(uint32_t *num_de)
{
    if (num_de == NULL) {
        return FWK_E_PARAM;
    }

    *num_de = telemetry_ctx.total_de_count;

    return FWK_SUCCESS;
}

static int shmti_id_to_ctx_index(uint32_t shmti_id, uint32_t *shmti_ctx_index)
{
    struct telemetry_shmti_context *shmti_ctx;
    uint32_t index;

    for (index = 0; index < telemetry_ctx.shmti_count; index++) {
        shmti_ctx = &telemetry_ctx.shmti_ctx_table[index];
        if (shmti_id == shmti_ctx->shmti_info->shmti_id) {
            *shmti_ctx_index = index;
            return FWK_SUCCESS;
        }
    }

    return FWK_E_PARAM;
}

/* Data Event and Event groups APIs. */
static bool get_de_source_list_index(
    struct telemetry_source_context *source_ctx,
    uint32_t de_id,
    uint32_t *source_de_list_index)
{
    unsigned int idx;

    if (source_ctx == NULL || source_de_list_index == NULL) {
        return false;
    }

    for (idx = 0; idx < source_ctx->num_de; idx++) {
        if (de_id == source_ctx->de_list[idx].de_id) {
            *source_de_list_index = idx;
            return true;
        }
    }

    return false;
}

static struct telemetry_source_context *get_source_for_de_id(
    uint32_t de_id,
    uint32_t *source_de_list_index)
{
    size_t source_index;
    struct telemetry_source_context *source_ctx;

    if (source_de_list_index == NULL) {
        return NULL;
    }

    for (source_index = 0; source_index < telemetry_ctx.num_sources;
         source_index++) {
        source_ctx = &telemetry_ctx.source_ctx_table[source_index];

        if (get_de_source_list_index(source_ctx, de_id, source_de_list_index)) {
            return source_ctx;
        }
    }

    return NULL;
}

const struct telemetry_source_context *get_source_for_de_index(
    uint32_t de_index,
    uint32_t *source_de_list_index)
{
    unsigned int i;
    uint32_t de_relative_index = de_index;
    struct telemetry_source_context *source_ctx;

    if (de_index >= telemetry_ctx.total_de_count) {
        return NULL;
    }

    /* Locate the DE within the source contexts */
    for (i = 0; i < telemetry_ctx.num_sources; i++) {
        source_ctx = &telemetry_ctx.source_ctx_table[i];
        if (source_ctx == NULL) {
            return NULL;
        }

        if (source_ctx->num_de <= de_relative_index) {
            de_relative_index -= source_ctx->num_de;
        } else {
            *source_de_list_index = de_relative_index;
            return source_ctx;
        }
    }

    return NULL; /* No matching telemetry source found */
}

static int telemetry_get_de_desc(
    uint32_t de_index,
    struct mod_telemetry_de_desc *de_desc)
{
    const struct telemetry_source_context *source_ctx;
    uint32_t source_de_list_index;

    /* Validate input parameters */
    if (de_desc == NULL) {
        return FWK_E_PARAM;
    }

    source_ctx = get_source_for_de_index(de_index, &source_de_list_index);
    if (source_ctx == NULL) {
        return FWK_E_PARAM;
    }
    memcpy(
        de_desc,
        &source_ctx->de_list[source_de_list_index],
        sizeof(struct mod_telemetry_de_desc));

    return FWK_SUCCESS;
}

static int telemetry_get_de_fch_attr(
    uint32_t de_index,
    struct mod_telemetry_de_fch_attr *de_fch_attr)
{
    const struct telemetry_source_context *source_ctx;
    uint32_t source_de_list_index;

    /* Validate input parameters */
    if (de_fch_attr == NULL) {
        return FWK_E_PARAM;
    }

    source_ctx = get_source_for_de_index(de_index, &source_de_list_index);
    if (source_ctx == NULL) {
        return FWK_E_PARAM;
    }

    return source_ctx->api->get_de_fch_attr(source_de_list_index, de_fch_attr);
}

static int telemetry_get_de_name(uint32_t de_index, char *name)
{
    const struct telemetry_source_context *source_ctx;
    uint32_t source_de_list_index;

    /* Validate input parameters */
    if (name == NULL) {
        return FWK_E_PARAM;
    }

    source_ctx = get_source_for_de_index(de_index, &source_de_list_index);
    if (source_ctx == NULL) {
        return FWK_E_PARAM;
    }
    return source_ctx->api->get_de_name(source_de_list_index, name);
}

static int telemetry_get_enabled_de_status(
    telemetry_de_handle_st de_handle,
    struct mod_telemetry_de_status *de_status)
{
    struct telemetry_source_context *source_ctx = NULL;
    uint8_t source_de_index = GET_SOURCE_DE_INDEX(de_handle);

    /* Validate input parameters */
    if (de_status == NULL || !VALIDATE_DE_HANDLE(de_handle)) {
        return FWK_E_PARAM;
    }

    source_ctx = &telemetry_ctx.source_ctx_table[GET_SOURCE_INDEX(de_handle)];
    if (source_ctx == NULL) {
        return FWK_E_PARAM;
    }

    /* Populate the DE status structure */
    de_status->de_id = source_ctx->de_list[source_de_index].de_id;
    de_status->ts_enabled =
        source_ctx->de_status_list[source_de_index].ts_enabled;

    return FWK_SUCCESS;
}

/*! Update Intervals APIs. */
static bool is_sampling_rate_valid(uint32_t sampling_rate)
{
    size_t idx;
    uint32_t min_rate, max_rate;

    /* Validate discrete update intervals */
    if (telemetry_ctx.config->interval_format ==
        MOD_TELEMETRY_UPDATE_INTERVALS_DISCRETE) {
        for (idx = 0; idx < telemetry_ctx.config->num_intervals; idx++) {
            if (sampling_rate == telemetry_ctx.config->sampling_rates[idx]) {
                return true;
            }
        }
        return false;
    }

    /* Validate linear update intervals */
    min_rate = telemetry_ctx.config->sampling_rates[0];
    max_rate = telemetry_ctx.config
                   ->sampling_rates[telemetry_ctx.config->num_intervals - 1];

    return (sampling_rate >= min_rate && sampling_rate <= max_rate);
}

static inline void write_de_data(
    const struct telemetry_source_context *source_ctx,
    unsigned int de_enabled_index)
{
    unsigned int i;
    uint32_t line_size;
    const struct mod_telemetry_de_data *de_data =
        &source_ctx->de_data_list[de_enabled_index];
    const struct telemetry_de_status *de_status =
        &source_ctx->de_status_list[de_data->source_de_index];
    const struct telemetry_shmti_context *shmti_ctx = de_status->curr_shmti_ctx;
    uint8_t *curr_addr =
        (uint8_t
             *)(shmti_ctx->shmti_info->start_addr + de_status->shmti_de_offset);
    /* Write first QWORD of data */
    if (de_status->ts_enabled && !source_ctx->config->uses_blk_ts) {
        line_size = MOD_TELEMETRY_DE_LINE_SIZE_TS;
        WRITE_DATA_LINE_LINE_TIMESTAMP(
            curr_addr,
            de_data->data[0],
            source_ctx->de_list[de_data->source_de_index].de_id,
            de_data->timestamp);
    } else {
        line_size = MOD_TELEMETRY_DE_LINE_SIZE_NON_TS;
        WRITE_DATA_LINE_NON_TIMESTAMP(
            curr_addr,
            de_data->data[0],
            source_ctx->de_list[de_data->source_de_index].de_id);
    }

    curr_addr = curr_addr + line_size;
    line_size = MOD_TELEMETRY_DE_LINE_SIZE_NON_TS;
    /* Write rest of the data in susequent lines. */
    for (i = 1; i < de_data->data_qword_count; ++i) {
        WRITE_DATA_LINE_NON_TIMESTAMP(
            curr_addr,
            de_data->data[i],
            source_ctx->de_list[de_data->source_de_index].de_id);
        curr_addr = curr_addr + line_size;
    }
}

static int telemetry_write_data(void)
{
    int status;
    int32_t shmti_iter;
    unsigned int i, de_idx;
    struct telemetry_source_context *source_ctx;

    /* Blocks agent read. */
    for (shmti_iter = 0; shmti_iter < (int32_t)telemetry_ctx.shmti_count;
         ++shmti_iter) {
        status = shmti_seq_inc(&telemetry_ctx.shmti_ctx_table[shmti_iter]);
        if (status != FWK_SUCCESS) {
            status = FWK_E_ACCESS;
            goto restore;
        }
    }

    /* Write all the updated data in SHMTI. */
    for (i = 0; i < telemetry_ctx.num_sources; ++i) {
        source_ctx = &telemetry_ctx.source_ctx_table[i];
        fwk_assert(source_ctx != NULL);
        if (source_ctx->de_enabled_count == 0) {
            continue;
        }
        for (de_idx = 0; de_idx < source_ctx->de_enabled_count; ++de_idx) {
            write_de_data(source_ctx, de_idx);
        }
    }

restore:
    /* Unblocks agent read for changed SHMTIs. */
    for (shmti_iter = shmti_iter - 1; shmti_iter >= 0; --shmti_iter) {
        status = shmti_seq_inc(&telemetry_ctx.shmti_ctx_table[shmti_iter]);
        if (status != FWK_SUCCESS) {
            return status;
        }
    }

    return FWK_SUCCESS;
}

static void update_telemetry_data(uintptr_t param)
{
    int status;
    unsigned int i;
    struct telemetry_source_context *source_ctx;

    /* Iterate through all telemetry sources and update their data */
    for (i = 0; i < telemetry_ctx.num_sources; ++i) {
        source_ctx = &telemetry_ctx.source_ctx_table[i];
        fwk_assert(source_ctx != NULL);

        /* Ensure the source API is valid and atleast one DE is enabled from
         * the source.
         */
        if (source_ctx->api == NULL || source_ctx->api->get_data == NULL ||
            source_ctx->de_enabled_count == 0) {
            continue;
        }

        /* Invoke the telemetry update function for the source */
        status = source_ctx->api->get_data(
            &source_ctx->de_data_list, source_ctx->de_enabled_count);
        if (status != FWK_SUCCESS ||
            (source_ctx->de_enabled_count > 0 &&
             source_ctx->de_data_list == NULL)) {
            FWK_LOG_CRIT("Failed to get data for Source (%u)", i);
        }
    }

    /* Write Data if all sources has been updated. */
    status = telemetry_write_data();
    if (status != FWK_SUCCESS) {
        FWK_LOG_CRIT("Failed to write data to interfaces");
    }
}

static int start_telemetry_update(uint32_t sampling_rate_ms)
{
    int status;

    /* Ensure telemetry is enabled before starting updates */
    if (!telemetry_ctx.telemetry_enabled) {
        return FWK_E_STATE;
    }

    if (telemetry_ctx.alarm_api == NULL) {
        return FWK_E_STATE;
    }

    /* Start the periodic telemetry update using the alarm API */
    status = telemetry_ctx.alarm_api->start(
        telemetry_ctx.config->alarm_id,
        sampling_rate_ms * 1000,
        MOD_TIMER_ALARM_TYPE_PERIODIC,
        update_telemetry_data,
        (uintptr_t)NULL);

    if (status != FWK_SUCCESS) {
        return status;
    }

    telemetry_ctx.is_telemetry_update_ongoing = true;

    return FWK_SUCCESS;
}

static int stop_telemetry_update(void)
{
    int status;

    /* Ensure telemetry is enabled before attempting to stop updates */
    if (!telemetry_ctx.telemetry_enabled || telemetry_ctx.alarm_api == NULL) {
        return FWK_E_STATE;
    }

    /* Stop the telemetry update using the alarm API */
    status = telemetry_ctx.alarm_api->stop(telemetry_ctx.config->alarm_id);
    if (status != FWK_SUCCESS) {
        return status;
    }

    telemetry_ctx.is_telemetry_update_ongoing = false;

    return FWK_SUCCESS;
}

static int telemetry_get_update_intervals_info(
    uint32_t *num_intervals,
    enum mod_telemetry_update_interval_formats *interval_format)
{
    /* Validate input parameters */
    if (num_intervals == NULL || interval_format == NULL) {
        return FWK_E_PARAM;
    }
    *num_intervals = telemetry_ctx.config->num_intervals;
    *interval_format = telemetry_ctx.config->interval_format;

    return FWK_SUCCESS;
}

static int telemetry_get_update_interval(
    uint32_t interval_index,
    uint32_t *interval)
{
    /* Validate input parameters */
    if (interval == NULL ||
        interval_index >= telemetry_ctx.config->num_intervals) {
        return FWK_E_PARAM;
    }
    *interval = telemetry_ctx.config->sampling_rates[interval_index];

    return FWK_SUCCESS;
}

static int set_sampling_rate(uint32_t sampling_rate_msecs)
{
    int status;

    /* Validate the requested sampling rate */
    if (!is_sampling_rate_valid(sampling_rate_msecs)) {
        return FWK_E_PARAM;
    }
    if (telemetry_ctx.current_sampling_rate_msecs == sampling_rate_msecs) {
        return FWK_SUCCESS;
    }

    /* If telemetry is enabled, restart the update process with the new rate */
    if (telemetry_ctx.telemetry_enabled) {
        status = stop_telemetry_update();
        if (status != FWK_SUCCESS) {
            return FWK_E_STATE;
        }
        status = start_telemetry_update(sampling_rate_msecs);
        if (status != FWK_SUCCESS) {
            return FWK_E_STATE;
        }
    }
    /* Update the telemetry context with the new sampling rate */
    telemetry_ctx.current_sampling_rate_msecs = sampling_rate_msecs;

    return FWK_SUCCESS;
}

static inline size_t calculate_shmti_bytes(
    uint32_t de_data_size,
    bool enable_ts,
    bool uses_blk_ts)
{
    size_t sz;
    uint32_t lines_required =
        de_data_size / MOD_TELEMETRY_DE_DATA_SIZE_PER_LINE;
    if (uses_blk_ts || !enable_ts) {
        sz = MOD_TELEMETRY_DE_LINE_SIZE_NON_TS;
    } else {
        sz = MOD_TELEMETRY_DE_LINE_SIZE_TS;
    }
    /* Only one line would need timestamp if enabled, NON_TS lines would
     * suffice for rest of the lines.
     */
    return sz + ((lines_required - 1) * MOD_TELEMETRY_DE_LINE_SIZE_NON_TS);
}

static inline void write_de_meta_data(
    uint32_t *addr,
    uint32_t id,
    uint32_t de_data_size,
    bool ts_enabled,
    bool uses_blk_ts)
{
    uint32_t *curr_addr;
    uint32_t line_size;
    bool line_ts_needs_to_be_written = false;
    uint32_t lines_required =
        de_data_size / MOD_TELEMETRY_DE_DATA_SIZE_PER_LINE;
    if (ts_enabled && !uses_blk_ts) {
        line_ts_needs_to_be_written = true;
        line_size = MOD_TELEMETRY_DE_LINE_SIZE_TS;
    } else {
        line_size = MOD_TELEMETRY_DE_LINE_SIZE_NON_TS;
    }

    while (lines_required != 0) {
        curr_addr =
            (uint32_t *)(((char *)addr) + ((lines_required - 1) * line_size));
        *curr_addr = METADATA_BUILD(
            false, uses_blk_ts, line_ts_needs_to_be_written, true);
        --lines_required;
    }
}

static int enable_event(
    uint32_t id,
    uint32_t shmti_id,
    bool enable_ts,
    telemetry_de_handle_st *de_handle,
    uint32_t *shmti_de_offset)
{
    int status;
    uint32_t source_de_list_index;
    uint32_t shmti_ctx_index;
    struct telemetry_source_context *source_ctx;
    struct telemetry_shmti_context *requested_shmti_ctx;
    uint32_t shmti_bytes_required;
    struct mod_telemetry_de_fch_attr fch_attr;

    /* Locate the source context for the DE */
    source_ctx = get_source_for_de_id(id, &source_de_list_index);
    if (source_ctx == NULL) {
        return FWK_E_PARAM;
    }
    status = source_ctx->api->get_de_fch_attr(source_de_list_index, &fch_attr);
    if (status != FWK_SUCCESS) {
        return FWK_E_PARAM;
    }
    /* DE via FCH is not suported yet. */
    if (fch_attr.fch_size != 0) {
        return FWK_E_SUPPORT;
    }

    /* Try to allocate event in SHMTI. */
    status = shmti_id_to_ctx_index(shmti_id, &shmti_ctx_index);
    if (status != FWK_SUCCESS) {
        return status;
    }
    requested_shmti_ctx = &telemetry_ctx.shmti_ctx_table[shmti_ctx_index];
    shmti_bytes_required = calculate_shmti_bytes(
        source_ctx->de_list[source_de_list_index].de_data_size,
        enable_ts,
        source_ctx->config->uses_blk_ts);
    status = shmti_alloc_pool(
        requested_shmti_ctx, shmti_bytes_required, shmti_de_offset);
    if (status != FWK_SUCCESS) {
        return FWK_E_NOMEM;
    }

    /* Enable the DE based on its timestamp requirement */
    status = source_ctx->api->enable_de(source_de_list_index);
    if (status != FWK_SUCCESS) {
        return status;
    }

    /* If enabling was successful, update the enabled DE list */
    source_ctx->de_enabled_count++;
    telemetry_ctx.total_de_enabled_count++;
    source_ctx->de_status_list[source_de_list_index].ts_enabled = enable_ts;
    source_ctx->de_status_list[source_de_list_index].curr_shmti_ctx =
        requested_shmti_ctx;
    source_ctx->de_status_list[source_de_list_index].shmti_de_offset =
        *shmti_de_offset;

    /* Write metadata for the DE in SHMTI. */
    write_de_meta_data(
        (uint32_t
             *)(requested_shmti_ctx->shmti_info->start_addr + *shmti_de_offset),
        id,
        source_ctx->de_list[source_de_list_index].de_data_size,
        enable_ts,
        source_ctx->config->uses_blk_ts);

    /*!
     * If this is the first enabled DE, start telemetry updating.
     */
    if (telemetry_ctx.total_de_enabled_count == 1 &&
        telemetry_ctx.telemetry_enabled) {
        status =
            start_telemetry_update(telemetry_ctx.current_sampling_rate_msecs);
        if (status != FWK_SUCCESS) {
            return status;
        }
    }
    /*
     * Construct the HAL identifier the enabled data event.
     * DE_HAL_ID = ID(SOURCE_ID, source_de_list_index)
     */
    CONSTRUCT_DE_HANDLE(*de_handle, source_ctx->index, 0, source_de_list_index);

    return FWK_SUCCESS;
}

static int telemetry_enable_de_non_ts(
    uint32_t de_id,
    uint32_t shmti_id,
    telemetry_de_handle_st *de_handle,
    uint32_t *shmti_de_offset)
{
    return enable_event(de_id, shmti_id, false, de_handle, shmti_de_offset);
}

static int telemetry_enable_de_ts(
    uint32_t de_id,
    uint32_t shmti_id,
    telemetry_de_handle_st *de_handle,
    uint32_t *shmti_de_offset)
{
    return enable_event(de_id, shmti_id, true, de_handle, shmti_de_offset);
}

static int telemetry_disable_de(telemetry_de_handle_st de_handle)
{
    int status;
    size_t shmti_bytes_allocated;
    struct telemetry_source_context *source_ctx;
    struct telemetry_de_status *de_status;

    if (!VALIDATE_DE_HANDLE(de_handle)) {
        return FWK_E_PARAM;
    }

    source_ctx = &telemetry_ctx.source_ctx_table[GET_SOURCE_INDEX(de_handle)];
    de_status = &source_ctx->de_status_list[GET_SOURCE_DE_INDEX(de_handle)];

    /* Disable the DE in the source. */
    status = source_ctx->api->disable_de(GET_SOURCE_DE_INDEX(de_handle));
    if (status != FWK_SUCCESS) {
        return status;
    }

    /* Deallocate the DE from shmti. */
    shmti_bytes_allocated = calculate_shmti_bytes(
        source_ctx->de_list[GET_SOURCE_DE_INDEX(de_handle)].de_data_size,
        de_status->ts_enabled,
        source_ctx->config->uses_blk_ts);
    status = shmti_free_pool(
        de_status->curr_shmti_ctx,
        shmti_bytes_allocated,
        de_status->shmti_de_offset);
    if (status != FWK_SUCCESS) {
        return status;
    }

    de_status->curr_shmti_ctx = NULL;
    de_status->shmti_de_offset = 0;
    source_ctx->de_enabled_count--;
    telemetry_ctx.total_de_enabled_count--;
    if (telemetry_ctx.total_de_enabled_count == 0) {
        /* Stop telemetry updates */
        status = stop_telemetry_update();
        if (status != FWK_SUCCESS) {
            return status;
        }
    }

    return FWK_SUCCESS;
}

static int telemetry_disable(void)
{
    int status;

    if (telemetry_ctx.is_telemetry_update_ongoing) {
        /* Stop telemetry updates */
        status = stop_telemetry_update();
        if (status != FWK_SUCCESS) {
            return status;
        }
    }
    /* Mark telemetry as disabled */
    telemetry_ctx.telemetry_enabled = false;

    return FWK_SUCCESS;
}

static int telemetry_enable(void)
{
    uint32_t sampling_rate;
    int status;
    bool begin_update = false;

    /* If telemetry is already enabled, return success */
    if (telemetry_ctx.telemetry_enabled) {
        return FWK_SUCCESS;
    }

    /* Determine the sampling rate to use */
    if (telemetry_ctx.current_sampling_rate_msecs == 0) {
        /* Use the default sampling rate if none is set */
        sampling_rate = telemetry_ctx.config->sampling_rates[0];
    } else {
        sampling_rate = telemetry_ctx.current_sampling_rate_msecs;
    }

    /* Start telemetry updates only if at least one DE is enabled */
    if (telemetry_ctx.total_de_enabled_count > 0) {
        begin_update = true;
    }

    /* Mark telemetry as enabled and store the sampling rate */
    telemetry_ctx.telemetry_enabled = true;
    telemetry_ctx.current_sampling_rate_msecs = sampling_rate;

    if (begin_update) {
        status = start_telemetry_update(sampling_rate);
        if (status != FWK_SUCCESS) {
            telemetry_ctx.telemetry_enabled = false;
            return status;
        }
    }

    return FWK_SUCCESS;
}

static int telemetry_reset(void)
{
    return FWK_SUCCESS;
}

static struct mod_telemetry_protocol_support_api scmi_protocol_support_api = {
    /* SHMTI APIs. */
    .shmti_init = telemetry_shmti_init,
    /* Data Event fetcher APIs. */
    .get_num_de = telemetry_get_num_de,
    .get_de_desc = telemetry_get_de_desc,
    .get_de_fch_desc = telemetry_get_de_fch_attr,
    .get_de_name = telemetry_get_de_name,
    .get_enabled_de_status = telemetry_get_enabled_de_status,
    /* Update intervals APIs. */
    .get_update_intervals_info = telemetry_get_update_intervals_info,
    .get_update_interval = telemetry_get_update_interval,
    .set_sampling_rate = set_sampling_rate,
    /* Data Event configure APIs. */
    .disable_de = telemetry_disable_de,
    .enable_de_non_ts = telemetry_enable_de_non_ts,
    .enable_de_ts = telemetry_enable_de_ts,
    /* Telemetry configure APIs. */
    .telemetry_disable = telemetry_disable,
    .telemetry_enable = telemetry_enable,
    .telemetry_reset = telemetry_reset
};

static int telemetry_init(
    fwk_id_t module_id,
    unsigned int source_count,
    const void *data)
{
    const struct mod_telemetry_config *config =
        (const struct mod_telemetry_config *)data;
    if (config == NULL) {
        return FWK_E_PARAM;
    }

    telemetry_ctx.source_ctx_table =
        fwk_mm_calloc(source_count, (sizeof(struct telemetry_source_context)));
    if (telemetry_ctx.source_ctx_table == NULL) {
        return FWK_E_NOMEM;
    }
    /*! Initialise telemetry context. */
    telemetry_ctx.config = config;
    telemetry_ctx.num_sources = source_count;
    telemetry_ctx.total_de_count = 0;
    telemetry_ctx.total_de_enabled_count = 0;
    telemetry_ctx.telemetry_enabled = false;
    telemetry_ctx.is_telemetry_update_ongoing = false;

    return FWK_SUCCESS;
}

static int telemetry_element_init(
    fwk_id_t source_id,
    unsigned int unused,
    const void *data)
{
    unsigned int source_idx;
    const struct mod_telemetry_source_config *config =
        (const struct mod_telemetry_source_config *)data;

    if (!fwk_module_is_valid_element_id(source_id) || config == NULL) {
        return FWK_E_PARAM;
    }

    if (config->uses_blk_ts) {
        /* BLK_TS is not supported yet.*/
        return FWK_E_SUPPORT;
    }

    source_idx = fwk_id_get_element_idx(source_id);
    telemetry_ctx.source_ctx_table[source_idx].index = source_idx;
    telemetry_ctx.source_ctx_table[source_idx].config = config;

    return FWK_SUCCESS;
}

static int telemetry_bind(fwk_id_t id, unsigned int round)
{
    struct telemetry_source_context *source_ctx;

    if (round == 0) {
        if (fwk_id_is_type(id, FWK_ID_TYPE_MODULE)) {
            /* Bind to the timer alarm API */
            return fwk_module_bind(
                telemetry_ctx.config->alarm_id,
                MOD_TIMER_API_ID_ALARM,
                &telemetry_ctx.alarm_api);
        }
        return FWK_SUCCESS;
    }

    /* Bind Round 1. */
    if (fwk_id_is_type(id, FWK_ID_TYPE_MODULE)) {
        return FWK_SUCCESS;
    }
    /* Source binding. */
    if (!fwk_module_is_valid_element_id(id) ||
        fwk_id_get_element_idx(id) >= telemetry_ctx.num_sources) {
        return FWK_E_PARAM;
    }

    source_ctx = &telemetry_ctx.source_ctx_table[fwk_id_get_element_idx(id)];
    return fwk_module_bind(
        FWK_ID_MODULE(fwk_id_get_module_idx(source_ctx->config->drv_api_id)),
        source_ctx->config->drv_api_id,
        &source_ctx->api);
}

static int telemetry_process_bind_request(
    fwk_id_t source_id,
    fwk_id_t target_id,
    fwk_id_t api_id,
    const void **api)
{
    if (api == NULL) {
        return FWK_E_PARAM;
    }

    enum mod_telemetry_api_idx api_idx =
        (enum mod_telemetry_api_idx)fwk_id_get_api_idx(api_id);

    if (api_idx != MOD_TELEMETRY_API_IDX_PROTOCOL_SUPPORT) {
        return FWK_E_PARAM;
    }
    *api = &scmi_protocol_support_api;

    return FWK_SUCCESS;
}

static int telemetry_start(fwk_id_t id)
{
    int status;
    struct telemetry_source_context *source_ctx;

    if (fwk_id_is_type(id, FWK_ID_TYPE_MODULE)) {
        return FWK_SUCCESS;
    }

    if (!fwk_module_is_valid_element_id(id) ||
        fwk_id_get_element_idx(id) >= telemetry_ctx.num_sources) {
        return FWK_E_PARAM;
    }

    source_ctx = &telemetry_ctx.source_ctx_table[fwk_id_get_element_idx(id)];
    status =
        source_ctx->api->get_de_list(&source_ctx->num_de, &source_ctx->de_list);
    if (status != FWK_SUCCESS) {
        return status;
    }

    source_ctx->de_status_list =
        fwk_mm_alloc(source_ctx->num_de, sizeof(struct telemetry_de_status));
    if (source_ctx->de_status_list == NULL) {
        return FWK_E_NOMEM;
    }
    source_ctx->de_data_list =
        fwk_mm_alloc(source_ctx->num_de, sizeof(struct mod_telemetry_de_data));
    if (source_ctx->de_data_list == NULL) {
        return FWK_E_NOMEM;
    }
    /* Update the total DE count */
    telemetry_ctx.total_de_count += source_ctx->num_de;

    return FWK_SUCCESS;
}

const struct fwk_module module_telemetry = {
    .type = FWK_MODULE_TYPE_HAL,
    .api_count = (unsigned int)MOD_TELEMETRY_API_IDX_COUNT,
    .init = telemetry_init,
    .element_init = telemetry_element_init,
    .bind = telemetry_bind,
    .process_bind_request = telemetry_process_bind_request,
    .start = telemetry_start,
};
