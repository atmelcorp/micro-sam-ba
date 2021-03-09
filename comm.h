/*
 * Copyright (c) 2015-2016, Atmel Corporation.
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms and conditions of the GNU General Public License,
 * version 2, as published by the Free Software Foundation.
 *
 * This program is distributed in the hope it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
 * more details.
 */

#ifndef COMM_H_
#define COMM_H_

#include <stdbool.h>
#include <stdint.h>

#if defined(_MSC_VER)
#define WINDOWS_LEAN_AND_MEAN
#include <windows.h>
#endif

#if defined(_MSC_VER)
typedef HANDLE handle_t;
#else
typedef int handle_t;
#define INVALID_HANDLE_VALUE -1
#endif

extern handle_t samba_open(const char* device);

extern void samba_close(handle_t fd);

extern bool samba_read_word(handle_t fd, uint32_t addr, uint32_t* value);

extern bool samba_write_word(handle_t fd, uint32_t addr, uint32_t value);

extern bool samba_read(handle_t fd, uint8_t* buffer, uint32_t addr, uint32_t size);

extern bool samba_write(handle_t fd, uint8_t* buffer, uint32_t addr, uint32_t size);

#endif /* COMM_H_ */
