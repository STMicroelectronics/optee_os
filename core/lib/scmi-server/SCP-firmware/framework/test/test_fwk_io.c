/*
 * Arm SCP/MCP Software
 * Copyright (c) 2025, Arm Limited and Contributors. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <fwk_assert.h>
#include <fwk_io.h>
#include <fwk_test.h>

#include <string.h>

#define FAKE_STR_SIZE 16

static char fake_str_out[FAKE_STR_SIZE];
static size_t fake_str_len;

enum fake_write_mode {
    FAKE_WRITE_FULL,
    FAKE_WRITE_PARTIAL,
    FAKE_WRITE_BUSY,
    FAKE_WRITE_ERROR,
};

static enum fake_write_mode write_mode;
static size_t write_partial_objs;

static int fake_write(
    const struct fwk_io_stream *stream,
    size_t *written,
    const void *buffer,
    size_t size,
    size_t count)
{
    *written = 0;

    switch (write_mode) {
    case FAKE_WRITE_FULL: {
        size_t bytes = count * size;
        memcpy(fake_str_out, buffer, bytes);
        fake_str_out[bytes] = '\0';
        fake_str_len = bytes;
        *written = count;
        return FWK_SUCCESS;
    }
    case FAKE_WRITE_PARTIAL: {
        size_t bytes = write_partial_objs * size;
        memcpy(fake_str_out, buffer, bytes);
        fake_str_out[bytes] = '\0';
        fake_str_len = bytes;
        *written = write_partial_objs;
        return FWK_SUCCESS;
    }
    case FAKE_WRITE_BUSY: {
        return FWK_E_BUSY;
    }
    case FAKE_WRITE_ERROR:
    default: {
        return FWK_E_HANDLER;
    }
    }
}

static int fake_putch(const struct fwk_io_stream *stream, char ch)
{
    fake_str_out[fake_str_len++] = ch;
    return FWK_SUCCESS;
}

/* Adapters */
static const struct fwk_io_adapter adapter_with_write = {
    .open = NULL,
    .getch = NULL,
    .putch = NULL,
    .write = fake_write,
    .close = NULL,
};

static const struct fwk_io_adapter adapter_with_putch = {
    .open = NULL,
    .getch = NULL,
    .putch = fake_putch,
    .write = NULL,
    .close = NULL,
};

/* Reset between tests */
static void test_case_setup(void)
{
    memset(fake_str_out, 0, sizeof(fake_str_out));
    fake_str_len = 0;
    write_mode = FAKE_WRITE_FULL;
    write_partial_objs = 0;
}

static void test_write_param_checks(void)
{
    struct fwk_io_stream s = { .adapter = &adapter_with_write };
    int status;
    const char buffer[] = "abc";
    size_t written, size = sizeof(char), count = sizeof(buffer) - 1;

    /* stream == NULL */
    status = fwk_io_write(NULL, &written, buffer, size, count);
    assert(status == FWK_E_PARAM);

    /* buffer == NULL */
    status = fwk_io_write(&s, &written, NULL, size, count);
    assert(status == FWK_E_PARAM);
}

static void test_write_adapter_null(void)
{
    struct fwk_io_stream s = { .adapter = NULL };
    const char buffer[] = "abc";
    size_t written, size = sizeof(char), count = sizeof(buffer) - 1;

    /* adapter == NULL */
    int status = fwk_io_write(&s, &written, buffer, size, count);
    assert(status == FWK_E_PARAM);
}

static void test_write_with_adapter_full_success(void)
{
    struct fwk_io_stream s = { .adapter = &adapter_with_write };
    int status;
    const char buffer[] = "HELLO";
    size_t written, size = sizeof(char), count = sizeof(buffer) - 1;

    write_mode = FAKE_WRITE_FULL;

    status = fwk_io_write(&s, &written, buffer, size, count);
    assert(status == FWK_SUCCESS);
    assert(written == count);
    assert(fake_str_len == (count * size));
    assert(memcmp(fake_str_out, buffer, (count * size)) == 0);
}

static void test_write_with_adapter_partial_success(void)
{
    struct fwk_io_stream s = { .adapter = &adapter_with_write };
    int status;
    const char buffer[] = "HELLOABCDE";
    size_t written, size = sizeof(char) * 2, count = sizeof(buffer) - 1;

    write_mode = FAKE_WRITE_PARTIAL;
    write_partial_objs = 3;

    status = fwk_io_write(&s, &written, buffer, size, count);
    assert(status == FWK_SUCCESS);
    assert(written == write_partial_objs);
    assert(fake_str_len == (write_partial_objs * size));
    assert(memcmp(fake_str_out, buffer, (write_partial_objs * size)) == 0);
}

static void test_write_with_adapter_busy(void)
{
    struct fwk_io_stream s = { .adapter = &adapter_with_write };
    int status;
    size_t written = 999; /* ensure it gets set properly */
    const char buffer[] = "HELLO";
    size_t size = sizeof(char), count = sizeof(buffer) - 1;

    write_mode = FAKE_WRITE_BUSY;

    status = fwk_io_write(&s, &written, buffer, size, count);
    assert(status == FWK_E_BUSY);
    assert(written == 0);
    assert(fake_str_len == 0);
}

static void test_write_with_adapter_error(void)
{
    struct fwk_io_stream s = { .adapter = &adapter_with_write };
    int status;
    size_t written = 0;
    const char buffer[] = "HELLO";
    size_t size = sizeof(char), count = sizeof(buffer) - 1;

    write_mode = FAKE_WRITE_ERROR;

    status = fwk_io_write(&s, &written, buffer, size, count);
    assert(status == FWK_E_HANDLER);
    /* 'written' is unspecified on error; fake doesn't modify it */
    assert(fake_str_len == 0);
}

static void test_write_fallback_to_putch_success(void)
{
    struct fwk_io_stream s = { .adapter = &adapter_with_putch,
                               .mode = FWK_IO_MODE_WRITE };
    int status;
    size_t written = 0;
    const char buffer[] = "HELLO";
    size_t size = sizeof(char), count = sizeof(buffer) - 1;

    status = fwk_io_write(&s, &written, buffer, size, count);
    assert(status == FWK_SUCCESS);
    assert(written == count);
    assert(fake_str_len == (count * size));
    assert(memcmp(fake_str_out, buffer, (count * size)) == 0);
}

static const struct fwk_test_case_desc test_case_table[] = {
    FWK_TEST_CASE(test_write_param_checks),
    FWK_TEST_CASE(test_write_adapter_null),
    FWK_TEST_CASE(test_write_with_adapter_full_success),
    FWK_TEST_CASE(test_write_with_adapter_partial_success),
    FWK_TEST_CASE(test_write_with_adapter_busy),
    FWK_TEST_CASE(test_write_with_adapter_error),
    FWK_TEST_CASE(test_write_fallback_to_putch_success),
};

struct fwk_test_suite_desc test_suite = {
    .name = "fwk_io",
    .test_case_setup = test_case_setup,
    .test_case_count = FWK_ARRAY_SIZE(test_case_table),
    .test_case_table = test_case_table,
};
