/*
 * Arm SCP/MCP Software
 * Copyright (c) 2024, Arm Limited and Contributors. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <errno.h>
#include <malloc.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <sys/stat.h>

extern int errno;

int _kill(int i, int j)
{
    errno = ENOSYS;
    return 0;
}

int _read(int fd, void *buf, int nbytes)
{
    errno = ENOSYS;
    return 0;
}

int _lseek(int file, int ptr, int dir)
{
    errno = ENOSYS;
    return 0;
}

int _close(int fd)
{
    errno = ENOSYS;
    return -1;
}

int _write(int fd, const void *buf, int nbytes)
{
    errno = ENOSYS;
    return 0;
}

int _getpid(void)
{
    errno = ENOSYS;
    return 0;
}

int _isatty(int file)
{
    errno = ENOSYS;
    return 0;
}

int _fstat(int file, struct stat *st)
{
    errno = ENOSYS;
    return 0;
}

void _exit(int rc)
{
    while (true) {
        __asm volatile("wfe");
    }
}
