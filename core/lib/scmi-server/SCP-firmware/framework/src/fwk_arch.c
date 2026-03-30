/*
 * Arm SCP/MCP Software
 * Copyright (c) 2015-2025, Arm Limited and Contributors. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 * Description:
 *     Framework API for the architecture layer.
 */
#include <internal/fwk_module.h>

#include <fwk_arch.h>
#include <fwk_assert.h>

#include <arch_interrupt.h>

#if FWK_HAS_INCLUDE(<fmw_arch.h>)
#    include <fmw_arch.h>
#endif

#include <internal/fwk_core.h>

#include <fwk_interrupt.h>
#include <fwk_io.h>
#include <fwk_log.h>
#include <fwk_module.h>
#include <fwk_module_idx.h>
#include <fwk_status.h>

#include <string.h>

int fwk_arch_init(void)
{
    int status;

    fwk_module_init();

    status = fwk_io_init();
    if (!fwk_expect(status == FWK_SUCCESS)) {
        return FWK_E_PANIC;
    }

    status = fwk_log_init();
    if (!fwk_expect(status == FWK_SUCCESS)) {
        return FWK_E_PANIC;
    }

    /* Initialize interrupt management */
    status = fwk_arch_interrupt_init();
    if (!fwk_expect(status == FWK_SUCCESS)) {
        return FWK_E_PANIC;
    }

    status = fwk_module_start();
    if (!fwk_expect(status == FWK_SUCCESS)) {
        return FWK_E_PANIC;
    }

    /*
     * In case firmware running under other OS context, finish processing of
     * any raised events/interrupts and return. Else continue to process events
     * in a forever loop.
     */
#if defined(BUILD_HAS_SUB_SYSTEM_MODE)
    fwk_process_event_queue();
    fwk_log_flush();
#else
    __fwk_run_main_loop();
#endif

    return FWK_SUCCESS;
}

int fwk_arch_deinit(void)
{
    int status;

    status = fwk_module_stop();
    if (!fwk_expect(status == FWK_SUCCESS)) {
        return FWK_E_PANIC;
    }

    return FWK_SUCCESS;
}

void fwk_arch_suspend(void)
{
    /* On some arm plaforms, wfe is supported architecturally, however
     * implementation is erroneous. In such platforms FMW_DISABLE_ARCH_SUSPEND
     * needs to be defined
     */
#if !defined(FMW_DISABLE_ARCH_SUSPEND)
    arch_suspend();
#endif
}
