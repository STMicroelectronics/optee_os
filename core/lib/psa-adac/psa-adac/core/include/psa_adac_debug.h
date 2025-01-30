/*
 * Copyright (c) 2020 Arm Limited. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef PSA_ADAC_DEBUG_H
#define PSA_ADAC_DEBUG_H

#include <stdio.h>
#include <stddef.h>
#include <stdint.h>

#include <psa_adac_config.h>

#define PSA_ADAC_LOG_LVL_NONE    50
#define PSA_ADAC_LOG_LVL_ERROR   40
#define PSA_ADAC_LOG_LVL_WARN    30
#define PSA_ADAC_LOG_LVL_INFO    20
#define PSA_ADAC_LOG_LVL_DEBUG   10
#define PSA_ADAC_LOG_LVL_TRACE   0
#ifndef PSA_ADAC_LOG_PRINT
#define PSA_ADAC_LOG_PRINT       printf
#endif
#ifdef PSA_ADAC_QUIET
#define PSA_ADAC_LOG_LEVEL       PSA_ADAC_LOG_LVL_NONE
#define PSA_ADAC_LOG_PRINTF(...)
#define PSA_ADAC_LOG_SPRINTF(var, format, ...)
#else
#define PSA_ADAC_LOG_PRINTF(...) printf(__VA_ARGS__)
#define PSA_ADAC_LOG_SPRINTF(var, format, ...) \
    sprintf(var, format, __VA_ARGS__)
#if defined(PSA_ADAC_TRACE)
#define PSA_ADAC_LOG_LEVEL       PSA_ADAC_LOG_LVL_TRACE
#elif defined(PSA_ADAC_DEBUG) || defined(DEBUG) || !defined(NDEBUG)
#define PSA_ADAC_LOG_LEVEL       PSA_ADAC_LOG_LVL_DEBUG
#else
#define PSA_ADAC_LOG_LEVEL       PSA_ADAC_LOG_LVL_INFO
#endif
#endif

#define PSA_ADAC_LOG_FUNC_AND_LEVEL(_level, _who) \
                    PSA_ADAC_LOG_PRINT("%-30.30s:% 5d : %-5.5s : %-10.10s : ", __func__, __LINE__, _level, _who)

#define PSA_ADAC_LOG_PRINT_LINE(_who, _level, _format, ...) \
    PSA_ADAC_LOG_PRINT("%-30.30s:% 5d : %-5.5s : %-10.10s : " _format, __func__, __LINE__, _who, _level, ##__VA_ARGS__)

#if PSA_ADAC_LOG_LEVEL <= PSA_ADAC_LOG_LVL_ERROR
#define PSA_ADAC_LOG_ERR(_who, _format, ...) PSA_ADAC_LOG_PRINT_LINE("error", _who, _format,  ##__VA_ARGS__)
#else
#define PSA_ADAC_LOG_ERR(_who, _format, ...) do{} while(0)
#endif

#if PSA_ADAC_LOG_LEVEL <= PSA_ADAC_LOG_LVL_WARN
#define PSA_ADAC_LOG_WARN(_who, _format, ...) PSA_ADAC_LOG_PRINT_LINE("warn", _who, _format,  ##__VA_ARGS__)
#else
#define PSA_ADAC_LOG_WARN(_who, _format, ...) do{} while(0)
#endif

#if PSA_ADAC_LOG_LEVEL <= PSA_ADAC_LOG_LVL_INFO
#define PSA_ADAC_LOG_INFO(_who, _format, ...) PSA_ADAC_LOG_PRINT_LINE("info", _who, _format,  ##__VA_ARGS__)
#else
#define PSA_ADAC_LOG_INFO(_who, _format, ...) do{} while(0)
#endif

#if PSA_ADAC_LOG_LEVEL <= PSA_ADAC_LOG_LVL_DEBUG
#define PSA_ADAC_LOG_DEBUG(_who, _format, ...) PSA_ADAC_LOG_PRINT_LINE("debug", _who, _format,  ##__VA_ARGS__)
#define PSA_ADAC_LOG_DUMP(_who, _label, _buff, _size)  \
    do { \
        uint32_t _i = 0, _j = 0, _k = 0; \
        for (_i = 0; _i * 16 + _j < _size; _i++, _j = 0) { \
            char _tmp[256] = {0}; \
            for (_j = 0, _k = 0; _i * 16 + _j < _size && _j < 16; _j++) { \
                _k += snprintf(_tmp + _k, 3, "%02x", ((uint8_t *)_buff)[_i * 16 + _j]); \
            } \
            PSA_ADAC_LOG_FUNC_AND_LEVEL("debug", _who); \
            PSA_ADAC_LOG_PRINT("%-10.10s %04x: %s\n", _label, _i * 16, _tmp); \
        } \
    } while(0)
#else
#define PSA_ADAC_LOG_DEBUG(_who, _format, ...) do{} while(0)
#define PSA_ADAC_LOG_DUMP(_who, _label, _buff, _size) do{} while(0)
#endif

#if PSA_ADAC_LOG_LEVEL <= PSA_ADAC_LOG_LVL_TRACE
#define PSA_ADAC_LOG_TRACE(_who, _format, ...) PSA_ADAC_LOG_PRINT_LINE("trace", _who, _format,  ##__VA_ARGS__)
#define PSA_ADAC_LOG_TDUMP(_who, _label, _buff, _size)  \
    do { \
        uint32_t _i = 0, _j = 0, _k = 0; \
        for (_i = 0; _i * 16 + _j < _size; _i++, _j = 0) { \
            char _tmp[256] = {0}; \
            for (_j = 0, _k = 0; _i * 16 + _j < _size && _j < 16; _j++) { \
                _k += snprintf(_tmp + _k, 3, "%02x", ((uint8_t *)_buff)[_i * 16 + _j]); \
            } \
            PSA_ADAC_LOG_FUNC_AND_LEVEL("trace", _who); \
            PSA_ADAC_LOG_PRINT("%-10.10s %04x: %s\n", _label, _i * 16, _tmp); \
        } \
    } while(0)
#else
#define PSA_ADAC_LOG_TRACE(_who, _format, ...) do{} while(0)
#define PSA_ADAC_LOG_TDUMP(_who, _label, _buff, _size) do{} while(0)
#endif

#define PSA_ADAC_ASSERT_ERROR(_cmd, _exp, _error) \
    do { \
        int _res = 0; \
        if ((_res = (int)(_cmd)) != _exp) { \
            PSA_ADAC_LOG_ERR(ENTITY_NAME, "failed to run[%s] res[%d] returning[%s]\n", #_cmd, _res, #_error); \
            res = _error; \
            goto bail; \
        } \
    } while (0)

#define PSA_ADAC_ASSERT(_cmd, _exp) \
    do { \
        if ((res = (_cmd)) != _exp) { \
            PSA_ADAC_LOG_ERR(ENTITY_NAME, "failed to run[%s] res[%d]\n", #_cmd, res); \
            goto bail; \
        } \
    } while (0)

#endif //PSA_ADAC_DEBUG_H
