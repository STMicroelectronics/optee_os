/*
 * Arm SCP/MCP Software
 * Copyright (c) 2023-2025, Arm Limited and Contributors. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include "config_scmi_power_capping_fast_channels_ut.h"
#include "scp_unity.h"
#include "unity.h"

#include <Mockfwk_mm.h>
#include <Mockfwk_module.h>
#include <Mockmod_power_capping_extra.h>
#include <Mockmod_transport_extra.h>
#include <Mockscmi_power_capping_core.h>
#include <internal/Mockfwk_core_internal.h>

#include <mod_scmi_power_capping_unit_test.h>

#include <stdarg.h>

#include UNIT_TEST_SRC

static struct pcapping_fast_channel_ctx fch_ctx_table[TEST_FCH_IDX_COUNT];

static const struct mod_transport_fast_channels_api transport_fch_api = {
    .transport_get_fch_address = transport_get_fch_address,
    .transport_get_fch_interrupt_type = transport_get_fch_interrupt_type,
    .transport_get_fch_doorbell_info = transport_get_fch_doorbell_info,
    .transport_get_fch_rate_limit = transport_get_fch_rate_limit,
    .transport_fch_register_callback = transport_fch_register_callback,
};

static uint32_t local_fast_channel_memory_emulation;
static uint32_t target_fast_channel_memory_emulation;

static uint32_t unsupported_domain_index;

/* Test functions */
/* Initialise the tests */

void init_test()
{
    uint32_t fch_idx = 0;
    while (fch_idx < TEST_FCH_IDX_COUNT) {
        for (fch_idx = 0; fch_idx < TEST_FCH_IDX_COUNT; fch_idx++) {
            if (unsupported_domain_index ==
                test_fch_config[fch_idx].scmi_power_capping_domain_idx) {
                unsupported_domain_index++;
                break;
            }
        }
    }
}

void setUp(void)
{
    uint32_t fch_idx;
    struct pcapping_fast_channel_ctx *fch_ctx;

    pcapping_fast_channel_global_ctx.fch_count = TEST_FCH_IDX_COUNT;
    pcapping_fast_channel_global_ctx.fch_ctx_table = fch_ctx_table;

    for (fch_idx = 0u; fch_idx < TEST_FCH_IDX_COUNT; fch_idx++) {
        fch_ctx = &(pcapping_fast_channel_global_ctx.fch_ctx_table[fch_idx]);
        fch_ctx->fch_config = &test_fch_config[fch_idx];
        fch_ctx->fch_address = (struct mod_transport_fast_channel_addr){
            .local_view_address =
                (uintptr_t)&local_fast_channel_memory_emulation,
            .target_view_address =
                (uintptr_t)&target_fast_channel_memory_emulation,
        };
        fch_ctx->transport_fch_api = &transport_fch_api;
    }
}

void tearDown(void)
{
}

void erase_setup(void)
{
    for (uint32_t fch_idx = 0u; fch_idx < TEST_FCH_IDX_COUNT; fch_idx++) {
        fch_ctx_table[fch_idx].fch_config = NULL;
    }
    pcapping_fast_channel_global_ctx.fch_count = 0;
    pcapping_fast_channel_global_ctx.fch_ctx_table = NULL;
}

void utest_pcapping_fast_channel_callback(void)
{
    uintptr_t param = (uintptr_t)NULL;

    __fwk_put_event_ExpectAnyArgsAndReturn(FWK_SUCCESS);
    pcapping_fast_channel_callback(param);
}

void utest_pcapping_fast_channel_process_command_cap_get(void)
{
    uint32_t fch_idx;
    uint32_t cap = __LINE__;

    for (fch_idx = TEST_FCH_IDX_CAP_GET_1; fch_idx <= TEST_FCH_IDX_CAP_GET_3;
         fch_idx++) {
        pcapping_core_get_cap_ExpectWithArrayAndReturn(
            test_fch_config[fch_idx].scmi_power_capping_domain_idx,
            &cap,
            sizeof(cap),
            FWK_SUCCESS);
        pcapping_core_get_cap_IgnoreArg_cap();
        pcapping_core_get_cap_ReturnMemThruPtr_cap(&cap, sizeof(cap));

        pcapping_fast_channel_process_command(fch_idx);
        TEST_ASSERT_EQUAL(local_fast_channel_memory_emulation, cap);
    }
}

void utest_pcapping_fast_channel_process_command_cap_set(void)
{
    uint32_t fch_idx;
    local_fast_channel_memory_emulation = __LINE__;

    for (fch_idx = TEST_FCH_IDX_CAP_SET_1; fch_idx <= TEST_FCH_IDX_CAP_SET_3;
         fch_idx++) {
        pcapping_core_set_cap_ExpectAndReturn(
            test_fch_config[fch_idx].service_id,
            test_fch_config[fch_idx].scmi_power_capping_domain_idx,
            true,
            local_fast_channel_memory_emulation,
            FWK_SUCCESS);

        pcapping_fast_channel_process_command(fch_idx);
    }
}

void utest_pcapping_fast_channel_process_command_pai_get(void)
{
    uint32_t fch_idx;
    uint32_t pai = __LINE__;

    for (fch_idx = TEST_FCH_IDX_PAI_GET_1; fch_idx <= TEST_FCH_IDX_PAI_GET_3;
         fch_idx++) {
        pcapping_core_get_pai_ExpectWithArrayAndReturn(
            test_fch_config[fch_idx].scmi_power_capping_domain_idx,
            &pai,
            sizeof(pai),
            FWK_SUCCESS);
        pcapping_core_get_pai_IgnoreArg_pai();
        pcapping_core_get_pai_ReturnMemThruPtr_pai(&pai, sizeof(pai));

        pcapping_fast_channel_process_command(fch_idx);
        TEST_ASSERT_EQUAL(local_fast_channel_memory_emulation, pai);
    }
}

void utest_pcapping_fast_channel_process_command_pai_set(void)
{
    uint32_t fch_idx;
    local_fast_channel_memory_emulation = __LINE__;

    for (fch_idx = TEST_FCH_IDX_PAI_SET_1; fch_idx <= TEST_FCH_IDX_PAI_SET_3;
         fch_idx++) {
        pcapping_core_set_pai_ExpectAndReturn(
            test_fch_config[fch_idx].service_id,
            test_fch_config[fch_idx].scmi_power_capping_domain_idx,
            local_fast_channel_memory_emulation,
            FWK_SUCCESS);
        pcapping_fast_channel_process_command(fch_idx);
    }
}

void utest_pcapping_fast_channel_process_event_hw_interrupt(void)
{
    int status;
    uint32_t fch_idx;
    struct fwk_event event;

    for (fch_idx = 0; fch_idx < TEST_FCH_IDX_COUNT; fch_idx++) {
        event.params[0] = fch_idx;
        pcapping_fast_channel_global_ctx.interrupt_type =
            MOD_TRANSPORT_FCH_INTERRUPT_TYPE_HW;

        switch (test_fch_config[fch_idx].message_id) {
        case MOD_SCMI_POWER_CAPPING_CAP_GET:
            pcapping_core_get_cap_ExpectWithArrayAndReturn(
                test_fch_config[fch_idx].scmi_power_capping_domain_idx,
                NULL,
                0,
                FWK_SUCCESS);
            pcapping_core_get_cap_IgnoreArg_cap();
            break;
        case MOD_SCMI_POWER_CAPPING_CAP_SET:
            pcapping_core_set_cap_ExpectAndReturn(
                test_fch_config[fch_idx].service_id,
                test_fch_config[fch_idx].scmi_power_capping_domain_idx,
                true,
                local_fast_channel_memory_emulation,
                FWK_SUCCESS);
            break;
        case MOD_SCMI_POWER_CAPPING_PAI_GET:
            pcapping_core_get_pai_ExpectWithArrayAndReturn(
                test_fch_config[fch_idx].scmi_power_capping_domain_idx,
                NULL,
                0,
                FWK_SUCCESS);
            pcapping_core_get_pai_IgnoreArg_pai();
            break;
        case MOD_SCMI_POWER_CAPPING_PAI_SET:
            pcapping_core_set_pai_ExpectAndReturn(
                test_fch_config[fch_idx].service_id,
                test_fch_config[fch_idx].scmi_power_capping_domain_idx,
                local_fast_channel_memory_emulation,
                FWK_SUCCESS);
            break;
        default:
            break;
        }

        status = pcapping_fast_channel_process_event(&event);
        TEST_ASSERT_EQUAL(status, FWK_SUCCESS);
    }
}

void utest_pcapping_fast_channel_process_event_timer_interrupt(void)
{
    int status;
    uint32_t fch_idx;
    struct fwk_event event;

    event = (struct fwk_event){};
    pcapping_fast_channel_global_ctx.interrupt_type =
        MOD_TRANSPORT_FCH_INTERRUPT_TYPE_TIMER;
    for (fch_idx = 0; fch_idx < TEST_FCH_IDX_COUNT; fch_idx++) {
        switch (test_fch_config[fch_idx].message_id) {
        case MOD_SCMI_POWER_CAPPING_CAP_GET:
            pcapping_core_get_cap_ExpectWithArrayAndReturn(
                test_fch_config[fch_idx].scmi_power_capping_domain_idx,
                NULL,
                0,
                FWK_SUCCESS);
            pcapping_core_get_cap_IgnoreArg_cap();
            break;
        case MOD_SCMI_POWER_CAPPING_CAP_SET:
            pcapping_core_set_cap_ExpectAndReturn(
                FWK_ID_NONE,
                test_fch_config[fch_idx].scmi_power_capping_domain_idx,
                true,
                local_fast_channel_memory_emulation,
                FWK_SUCCESS);
            break;
        case MOD_SCMI_POWER_CAPPING_PAI_GET:
            pcapping_core_get_pai_ExpectWithArrayAndReturn(
                test_fch_config[fch_idx].scmi_power_capping_domain_idx,
                NULL,
                0,
                FWK_SUCCESS);
            pcapping_core_get_pai_IgnoreArg_pai();
            break;
        case MOD_SCMI_POWER_CAPPING_PAI_SET:
            pcapping_core_set_pai_ExpectAndReturn(
                FWK_ID_NONE,
                test_fch_config[fch_idx].scmi_power_capping_domain_idx,
                local_fast_channel_memory_emulation,
                FWK_SUCCESS);
            break;
        default:
            break;
        }
    }
    status = pcapping_fast_channel_process_event(&event);
    TEST_ASSERT_EQUAL(status, FWK_SUCCESS);
}

void utest_pcapping_fast_channel_ctx_init_success(void)
{
    erase_setup();

    fwk_mm_calloc_ExpectAndReturn(
        TEST_FCH_IDX_COUNT,
        sizeof(struct pcapping_fast_channel_ctx),
        fch_ctx_table);

    int status =
        pcapping_fast_channel_ctx_init(test_fch_config, TEST_FCH_IDX_COUNT);

    TEST_ASSERT_EQUAL(status, FWK_SUCCESS);
    TEST_ASSERT_EQUAL(
        pcapping_fast_channel_global_ctx.fch_count, TEST_FCH_IDX_COUNT);
    TEST_ASSERT_EQUAL_PTR(
        pcapping_fast_channel_global_ctx.fch_ctx_table, fch_ctx_table);

    for (uint32_t fch_idx = 0; fch_idx < TEST_FCH_IDX_COUNT; fch_idx++) {
        TEST_ASSERT_EQUAL_PTR(
            pcapping_fast_channel_global_ctx.fch_ctx_table[fch_idx].fch_config,
            &test_fch_config[fch_idx]);
    }
}

void utest_pcapping_fast_channel_ctx_init_out_of_range(void)
{
    struct scmi_pcapping_fch_config fch_config;

    for (unsigned int message_id = MOD_SCMI_PROTOCOL_VERSION;
         message_id < MOD_SCMI_POWER_CAPPING_CAP_GET;
         message_id++) {
        fwk_mm_calloc_ExpectAndReturn(
            1,
            sizeof(struct pcapping_fast_channel_ctx),
            pcapping_fast_channel_global_ctx.fch_ctx_table);
        fch_config.message_id = message_id;
        int status = pcapping_fast_channel_ctx_init(&fch_config, 1);
        TEST_ASSERT_EQUAL(status, FWK_E_RANGE);
    }

    for (unsigned int message_id = MOD_SCMI_POWER_CAPPING_DOMAIN_NAME_GET;
         message_id < MOD_SCMI_POWER_CAPPING_COMMAND_COUNT;
         message_id++) {
        fwk_mm_calloc_ExpectAndReturn(
            1,
            sizeof(struct pcapping_fast_channel_ctx),
            pcapping_fast_channel_global_ctx.fch_ctx_table);

        fch_config.message_id = message_id;
        int status = pcapping_fast_channel_ctx_init(&fch_config, 1);
        TEST_ASSERT_EQUAL(status, FWK_E_RANGE);
    }
}

void utest_pcapping_fast_channel_bind(void)
{
    int status;
    uint32_t fch_idx;
    struct pcapping_fast_channel_ctx *fch_ctx;

    for (fch_idx = 0; fch_idx < TEST_FCH_IDX_COUNT; fch_idx++) {
        fch_ctx = &(pcapping_fast_channel_global_ctx.fch_ctx_table[fch_idx]);
        fwk_module_bind_ExpectAndReturn(
            fch_ctx->fch_config->transport_id,
            fch_ctx->fch_config->transport_api_id,
            &(fch_ctx->transport_fch_api),
            FWK_SUCCESS);
    }

    status = pcapping_fast_channel_bind();
    TEST_ASSERT_EQUAL(status, FWK_SUCCESS);
}

void utest_pcapping_fast_channel_start(void)
{
    uint32_t fch_idx;
    const struct scmi_pcapping_fch_config *fch_config;
    struct pcapping_fast_channel_ctx *fch_ctx;

    for (fch_idx = 0; fch_idx < TEST_FCH_IDX_COUNT; fch_idx++) {
        fch_ctx = &(pcapping_fast_channel_global_ctx.fch_ctx_table[fch_idx]);
        fch_config = fch_ctx->fch_config;
        transport_get_fch_address_ExpectAndReturn(
            fch_config->transport_id, &(fch_ctx->fch_address), FWK_SUCCESS);
        transport_get_fch_rate_limit_ExpectAndReturn(
            fch_config->transport_id, &(fch_ctx->fch_rate_limit), FWK_SUCCESS);
        transport_get_fch_interrupt_type_ExpectAndReturn(
            fch_config->transport_id,
            &(pcapping_fast_channel_global_ctx.interrupt_type),
            FWK_SUCCESS);
        transport_fch_register_callback_ExpectAndReturn(
            fch_config->transport_id,
            (uintptr_t)fch_idx,
            pcapping_fast_channel_callback,
            FWK_SUCCESS);
    }

    pcapping_fast_channel_start();
}

#ifdef BUILD_HAS_SCMI_POWER_CAPPING_STD_COMMANDS
void utest_pcapping_fast_channel_get_domain_support_true(void)
{
    for (uint32_t fch_idx = 0; fch_idx < TEST_FCH_IDX_COUNT; fch_idx++) {
        uint32_t supported_domain_index =
            test_fch_config[fch_idx].scmi_power_capping_domain_idx;
        bool support =
            pcapping_fast_channel_get_domain_support(supported_domain_index);

        TEST_ASSERT_EQUAL(support, true);
    }
}

void utest_pcapping_fast_channel_get_domain_support_false(void)
{
    bool support =
        pcapping_fast_channel_get_domain_support(unsupported_domain_index);

    TEST_ASSERT_EQUAL(support, false);
}

void utest_pcapping_fast_channel_get_channel_ctx_found(void)
{
    for (uint32_t fch_idx = 0; fch_idx < TEST_FCH_IDX_COUNT; fch_idx++) {
        struct pcapping_fast_channel_ctx *fch_ctx = get_channel_ctx(
            test_fch_config[fch_idx].scmi_power_capping_domain_idx,
            test_fch_config[fch_idx].message_id);
        TEST_ASSERT_EQUAL_PTR(
            fch_ctx,
            &(pcapping_fast_channel_global_ctx.fch_ctx_table[fch_idx]));
    }
}

void utest_pcapping_fast_channels_get_channel_ctx_not_found(void)
{
    struct pcapping_fast_channel_ctx *fch_ctx = get_channel_ctx(
        test_fch_config[0].scmi_power_capping_domain_idx,
        MOD_SCMI_PROTOCOL_VERSION);
    TEST_ASSERT_EQUAL_PTR(fch_ctx, NULL);

    fch_ctx = get_channel_ctx(
        unsupported_domain_index, test_fch_config[0].message_id);
    TEST_ASSERT_EQUAL_PTR(fch_ctx, NULL);
}

#endif

int scmi_test_main(void)
{
    UNITY_BEGIN();
    init_test();
    RUN_TEST(utest_pcapping_fast_channel_callback);
    RUN_TEST(utest_pcapping_fast_channel_process_command_cap_get);
    RUN_TEST(utest_pcapping_fast_channel_process_command_cap_set);
    RUN_TEST(utest_pcapping_fast_channel_process_command_pai_get);
    RUN_TEST(utest_pcapping_fast_channel_process_command_pai_set);
    RUN_TEST(utest_pcapping_fast_channel_process_event_hw_interrupt);
    RUN_TEST(utest_pcapping_fast_channel_process_event_timer_interrupt);
    RUN_TEST(utest_pcapping_fast_channel_ctx_init_success);
    RUN_TEST(utest_pcapping_fast_channel_ctx_init_out_of_range);
    RUN_TEST(utest_pcapping_fast_channel_bind);
    RUN_TEST(utest_pcapping_fast_channel_start);
#ifdef BUILD_HAS_SCMI_POWER_CAPPING_STD_COMMANDS
    RUN_TEST(utest_pcapping_fast_channel_get_domain_support_true);
    RUN_TEST(utest_pcapping_fast_channel_get_domain_support_false);
    RUN_TEST(utest_pcapping_fast_channel_get_channel_ctx_found);
    RUN_TEST(utest_pcapping_fast_channels_get_channel_ctx_not_found);
#endif
    return UNITY_END();
}

#if !defined(TEST_ON_TARGET)
int main(void)
{
    return scmi_test_main();
}
#endif
