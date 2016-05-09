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

#ifndef CHIPID_H_
#define CHIPID_H_

#include <stdbool.h>
#include <stdint.h>

struct _chip {
	const char* name;
	uint32_t    cidr;
	uint32_t    exid;
	uint32_t    eefc_base;
	uint32_t    flash_addr;
	uint32_t    flash_size;
	uint8_t     gpnvm;
};

struct _chip_serie {
	const char*         name;
	uint32_t            cidr_reg;
	uint32_t            exid_reg;
	uint32_t            nb_chips;
	const struct _chip* chips;
};

extern const struct _chip_serie* chipid_get_serie(const char* name);

extern bool chipid_check_serie(int fd, const struct _chip_serie* serie, const struct _chip** chip);

extern const struct _chip_serie* chipid_identity_serie(int fd, const struct _chip** chip);

#endif /* CHIPID_H_ */
