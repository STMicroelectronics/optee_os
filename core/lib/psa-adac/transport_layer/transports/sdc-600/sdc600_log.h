/*
 * Copyright (c) 2016-2019, Arm Limited. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 */

#ifndef SDC600_LOG_H_
#define SDC600_LOG_H_

#include "psa_adac_debug.h"
#include <stdio.h>
#include <stddef.h>
#include <stdint.h>

// Map SDC600 log macros to PSA_ADAC log macros.

#define SDC600_TRACE           0

#define SDC600_LOG_FUNC_AND_LEVEL(_level, _who) PSA_ADAC_LOG_FUNC_AND_LEVEL(_level, _who)

#define SDC600_LOG_PRINT_LINE(_who, _level, _format, ...) PSA_ADAC_LOG_PRINT_LINE(_who, _level, _format, ##__VA_ARGS__)

#define DEBUG_REG(_a) \
                SDC600_LOG_DEBUG("TEST", "%s: [secure][%p] = [0x%08x]; [non-secure][%p] = [0x%08x]\n", \
                #_a, \
                (uint32_t *)APBCOM_ADDR(_a), \
                (uint32_t)APBCOM_READ_WORD(_a), \
                (uint32_t *)APBCOM_ADDR(_a - 0x10000000), \
                (uint32_t)APBCOM_READ_WORD(_a - 0x10000000));

#define SDC600_LOG_ERR(_who, _format, ...) PSA_ADAC_LOG_ERR(_who, _format,  ##__VA_ARGS__)

#define SDC600_LOG_WARN(_who, _format, ...) PSA_ADAC_LOG_WARN(_who, _format,  ##__VA_ARGS__)

#define SDC600_LOG_INFO(_who, _format, ...) PSA_ADAC_LOG_INFO(_who, _format,  ##__VA_ARGS__)

#define SDC600_LOG_DEBUG(_who, _format, ...) PSA_ADAC_LOG_DEBUG(_who, _format,  ##__VA_ARGS__)

#define SDC600_LOG_BUF(_who, _buff, _size, _label)  PSA_ADAC_LOG_DUMP(_who, _label, _buff, _size)

#define SDC600_ASSERT_ERROR(_cmd, _exp, _error) \
                do { \
                    int _res = 0; \
                    if (SDC600_TRACE) SDC600_LOG_DEBUG(ENTITY_NAME, "running[%s]\n", #_cmd); \
                    if ((_res = (int)(_cmd)) != _exp) \
                    { \
                        SDC600_LOG_ERR(ENTITY_NAME, "failed to run[%s] res[%d] returning[0x%08x]\n", #_cmd, _res, _error); \
                        res = _error; \
                        goto bail; \
                    } \
                } while (0)

#define SDC600_ASSERT(_cmd, _exp) \
                         do { \
                    int _res = 0; \
                    if (SDC600_TRACE) SDC600_LOG_DEBUG(ENTITY_NAME, "running[%s]\n", #_cmd); \
                    if ((_res = (int)(_cmd)) != _exp) \
                    { \
                        SDC600_LOG_ERR(ENTITY_NAME, "failed to run[%s] res[%d]\n", #_cmd, _res); \
                        res = _res; \
                        goto bail; \
                    } \
                } while (0)

#endif /* SDC600_LOG_H_ */
