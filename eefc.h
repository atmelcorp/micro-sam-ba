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

#ifndef EEFC_H_
#define EEFC_H_

#include <stdbool.h>
#include <stdint.h>

#include "common_types.h"

#define MAX_EEFC_LOCKS 256

struct _chip;

struct _eefc_locks {
	uint32_t count;
	uint32_t size[MAX_EEFC_LOCKS];
};

extern bool eefc_read_flash_info(serial_port_handle_t fd, const struct _chip* chip,
		struct _eefc_locks* locks);

extern bool eefc_lock_page(serial_port_handle_t fd, const struct _chip* chip,
		const struct _eefc_locks* locks, uint32_t page);

extern bool eefc_unlock_page(serial_port_handle_t fd, const struct _chip* chip,
		const struct _eefc_locks* locks, uint32_t page);

extern bool eefc_lock(serial_port_handle_t fd, const struct _chip* chip,
		const struct _eefc_locks* locks, uint32_t addr, uint32_t size);

extern bool eefc_unlock(serial_port_handle_t fd, const struct _chip* chip,
		const struct _eefc_locks* locks, uint32_t addr, uint32_t size);

extern bool eefc_erase_all(serial_port_handle_t fd, const struct _chip* chip);

extern bool eefc_erase_16pages(serial_port_handle_t fd, const struct _chip* chip,
		uint32_t first_page);

extern bool eefc_read(serial_port_handle_t fd, const struct _chip* chip,
		uint8_t* buffer, uint32_t addr, uint32_t size);

extern bool eefc_write(serial_port_handle_t fd, const struct _chip* chip,
		uint8_t* buffer, uint32_t addr, uint32_t size);

extern bool eefc_get_gpnvm(serial_port_handle_t fd, const struct _chip* chip,
		uint8_t gpnvm, bool* value);

extern bool eefc_set_gpnvm(serial_port_handle_t fd, const struct _chip* chip, uint8_t gpnvm);

extern bool eefc_clear_gpnvm(serial_port_handle_t fd, const struct _chip* chip, uint8_t gpnvm);

#endif /* EEFC_H_ */
