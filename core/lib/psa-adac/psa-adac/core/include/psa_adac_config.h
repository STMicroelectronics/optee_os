/* SPDX-License-Identifier: BSD-3-Clause */
/*
 * Copyright (c) 2020-2025 Arm Limited. All rights reserved.
 * Copyright (c) 2025 STMicroelectronic
 */

#include <trace_levels.h>

#define PSA_ADAC_TARGET
#define PSA_ADAC_EC_P256
#if defined(CFG_PSA_ADAC_AUTHENTICATOR_IMPLICIT_TRANSPORT)
#define PSA_ADAC_AUTHENTICATOR_IMPLICIT_TRANSPORT
#endif

#if (CFG_TEE_CORE_LOG_LEVEL == TRACE_DEBUG)
#define PSA_ADAC_DEBUG /* Used to set PSA_ADAC_LOG_LEVEL to Debug */
#elif (CFG_TEE_CORE_LOG_LEVEL == TRACE_FLOW)
#define PSA_ADAC_TRACE /* Used to set PSA_ADAC_LOG_LEVEL to Trace */
#elif (CFG_TEE_CORE_LOG_LEVEL == TRACE_MIN)
#define PSA_ADAC_QUIET /* Used to disable console output */
#endif
