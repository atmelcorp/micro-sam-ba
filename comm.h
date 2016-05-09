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

extern int samba_open(const char* device);

extern void samba_close(int fd);

extern bool samba_read_word(int fd, uint32_t addr, uint32_t* value);

extern bool samba_write_word(int fd, uint32_t addr, uint32_t value);

extern bool samba_read(int fd, uint8_t* buffer, uint32_t addr, uint32_t size);

extern bool samba_write(int fd, uint8_t* buffer, uint32_t addr, uint32_t size);

#endif /* COMM_H_ */
