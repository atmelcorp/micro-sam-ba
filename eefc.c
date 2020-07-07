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

#include <stdio.h>
#include <stdlib.h>
#include "chipid.h"
#include "comm.h"
#include "eefc.h"
#include "utils.h"

#define PAGE_SIZE 512

#define EEFC_FMR  0x00000000
#define EEFC_FCR  0x00000004
#define EEFC_FSR  0x00000008
#define EEFC_FRR  0x0000000c

#define EEFC_FCR_FKEY      (0x5a << 24)
#define EEFC_FCR_FCMD_GETD 0x00 // Get Flash descriptor
#define EEFC_FCR_FCMD_WP   0x01 // Write page
#define EEFC_FCR_FCMD_WPL  0x02 // Write page and lock
#define EEFC_FCR_FCMD_EWP  0x03 // Erase page and write page
#define EEFC_FCR_FCMD_EWPL 0x04 // Erase page and write page then lock
#define EEFC_FCR_FCMD_EA   0x05 // Erase all
#define EEFC_FCR_FCMD_EPA  0x07 // Erase pages
#define EEFC_FCR_FCMD_SLB  0x08 // Set lock bit
#define EEFC_FCR_FCMD_CLB  0x09 // Clear lock bit
#define EEFC_FCR_FCMD_GLB  0x0A // Get lock bit
#define EEFC_FCR_FCMD_SGPB 0x0B // Set GPNVM bit
#define EEFC_FCR_FCMD_CGPB 0x0C // Clear GPNVM bit
#define EEFC_FCR_FCMD_GGPB 0x0D // Get GPNVM bit

#define EEFC_FSR_FRDY   (1 << 0)
#define EEFC_FSR_CMDE   (1 << 1)
#define EEFC_FSR_FLOCKE (1 << 2)
#define EEFC_FSR_FLERR  (1 << 3)

static bool eefc_wait_ready(int fd, const struct _chip* chip, uint32_t* status)
{
	uint32_t value;
	do {
		if (!samba_read_word(fd, chip->eefc_base + EEFC_FSR, &value))
			return false;
	} while (!(value & EEFC_FSR_FRDY));
	if (status)
		*status = value;
	return true;
}

static bool eefc_read_result(int fd, const struct _chip* chip, uint32_t* result)
{
	return samba_read_word(fd, chip->eefc_base + EEFC_FRR, result);
}

static bool eefc_send_command(int fd, const struct _chip* chip, uint8_t cmd,
		uint16_t arg, uint32_t* status)
{
	if (!samba_write_word(fd, chip->eefc_base + EEFC_FCR,
				EEFC_FCR_FKEY | (arg << 8) | cmd))
		return false;

	if (!eefc_wait_ready(fd, chip, status))
		return false;

	return true;
}

bool eefc_read_flash_info(int fd, const struct _chip* chip,
		struct _eefc_locks* locks)
{
	// send GETD command
	if (!eefc_send_command(fd, chip, EEFC_FCR_FCMD_GETD, 0, NULL))
		return false;

	// flash ID (discarded)
	uint32_t flash_id;
	if (!eefc_read_result(fd, chip, &flash_id))
		return false;

	// flash size
	uint32_t flash_size;
	if (!eefc_read_result(fd, chip, &flash_size))
		return false;
	if (flash_size != chip->flash_size * 1024) {
		fprintf(stderr, "Invalid flash size: detected %d bytes but expected %d bytes\n",
				flash_size, chip->flash_size * 1024);
		return false;
	}

	// page size
	uint32_t page_size;
	if (!eefc_read_result(fd, chip, &page_size))
		return false;
	if (page_size != PAGE_SIZE) {
		fprintf(stderr, "Invalid page size: detected %d bytes but expected %d bytes\n",
				page_size, PAGE_SIZE);
		return false;
	}

	// number of planes and plane sizes (discarded)
	uint32_t nb_planes, plane_size;
	if (!eefc_read_result(fd, chip, &nb_planes))
		return false;
	for (int i = 0; i < nb_planes; i++)
		if (!eefc_read_result(fd, chip, &plane_size))
			return false;

	// number of locks and lock sizes
	if (!eefc_read_result(fd, chip, &locks->count))
		return false;
	if (locks->count > MAX_EEFC_LOCKS)
		return false;
	for (int i = 0; i < locks->count; i++)
		if (!eefc_read_result(fd, chip, &locks->size[i]))
			return false;

	return true;
}

static bool set_page_lock(int fd, const struct _chip* chip,
		const struct _eefc_locks* locks, uint32_t lock, bool enable)
{
	if (lock > locks->count)
		return false;

	uint8_t cmd = enable ? EEFC_FCR_FCMD_SLB : EEFC_FCR_FCMD_CLB;
	return eefc_send_command(fd, chip, cmd, lock, NULL);
}

bool eefc_lock_page(int fd, const struct _chip* chip,
		const struct _eefc_locks* locks, uint32_t lock)
{
	return set_page_lock(fd, chip, locks, lock, true);
}

bool eefc_unlock_page(int fd, const struct _chip* chip,
		const struct _eefc_locks* locks, uint32_t lock)
{
	return set_page_lock(fd, chip, locks, lock, false);
}

static bool set_lock(int fd, const struct _chip* chip,
		const struct _eefc_locks* locks, uint32_t addr, uint32_t size,
		bool enable)
{
	if (addr + size > chip->flash_size * 1024)
		return false;

	uint32_t addr_end = addr + size;

	uint32_t offset = 0;
	for (int lock = 0; lock < locks->count; lock++) {
		uint32_t next_offset = offset + locks->size[lock];
		if (addr >= offset && addr < next_offset) {
			if (!set_page_lock(fd, chip, locks, lock, enable))
				return false;
			addr = next_offset;
			if (addr >= addr_end)
				return true;
		}
		offset = next_offset;
	}
	return false;
}

bool eefc_lock(int fd, const struct _chip* chip,
		const struct _eefc_locks* locks, uint32_t addr, uint32_t size)
{
	return set_lock(fd, chip, locks, addr, size, true);
}

bool eefc_unlock(int fd, const struct _chip* chip,
		const struct _eefc_locks* locks, uint32_t addr, uint32_t size)
{
	return set_lock(fd, chip, locks, addr, size, false);
}

bool eefc_erase_all(int fd, const struct _chip* chip)
{
	// send erase all command to flash controller
	uint32_t status;
	if (!eefc_send_command(fd, chip, EEFC_FCR_FCMD_EA, 0, &status))
		return false;
	if (status & EEFC_FSR_FLOCKE) {
		fprintf(stderr, "Erase error: at least one page is locked\n");
		return false;
	}
	if (status & EEFC_FSR_FLERR) {
		fprintf(stderr, "Erase error: flash error\n");
		return false;
	}
	return true;
}

bool eefc_erase_16pages(int fd, const struct _chip* chip,
		uint32_t first_page)
{
	uint32_t arg;

	if (first_page & 15) {
		fprintf(stderr, "Erase pages error: first page must be multiple of 16\n");
		return false;
	}

	arg = (first_page & ~15) | 2;

	// send erase pages command to flash controller
	uint32_t status;
	if (!eefc_send_command(fd, chip, EEFC_FCR_FCMD_EPA, arg, &status))
		return false;
	if (status & EEFC_FSR_FLOCKE) {
		fprintf(stderr, "Erase pages error: at least one page is locked\n");
		return false;
	}
	if (status & EEFC_FSR_FLERR) {
		fprintf(stderr, "Erase pages error: flash error\n");
		return false;
	}

	return true;
}

bool eefc_read(int fd, const struct _chip* chip,
		uint8_t* buffer, uint32_t addr, uint32_t size)
{
	if (addr + size > chip->flash_size * 1024){
		printf("invalid addr/size vs chip size parameter\n");
		printf("addr/size is %#x\n", (addr + size));
		printf("chip size is %#x\n", chip->flash_size * 1024);
		printf("workaround active!!!\n");
		// return false;
	}
	return samba_read(fd, buffer, chip->flash_addr + addr, size);
}

bool eefc_write(int fd, const struct _chip* chip,
		uint8_t* buffer, uint32_t addr, uint32_t size)
{
	if (addr + size > chip->flash_size * 1024)
		return false;

	while (size > 0) {
		uint16_t page = addr / PAGE_SIZE;
		uint32_t head = addr & (PAGE_SIZE - 1);
		uint32_t count = MIN(size, PAGE_SIZE - head);

		printf("writing addr %#.8x\n", addr);

		// write to latch buffer
		// we cannot use the SAM-BA Monitor send command because it
		// does byte writes and the flash controller needs word writes
		uint32_t* wbuffer = (uint32_t*)buffer;
		for (uint32_t i = 0; i < (count + 3) / 4; i++)
			if (!samba_write_word(fd, chip->flash_addr + addr + i * 4, wbuffer[i]))
				return false;

		// send write command to flash controller
		uint32_t status;
		if (!eefc_send_command(fd, chip, EEFC_FCR_FCMD_WP, page, &status))
			return false;
		if (status & EEFC_FSR_FLOCKE) {
			fprintf(stderr, "Write error on page %d: page locked\n", page);
			return false;
		}
		if (status & EEFC_FSR_FLERR) {
			fprintf(stderr, "Write error on page %d: flash error\n", page);
			return false;
		}

		buffer += count;
		addr += count;
		size -= count;
	}

	return true;
}

extern bool eefc_get_gpnvm(int fd, const struct _chip* chip,
		uint8_t gpnvm, bool* value)
{
	if (gpnvm >= chip->gpnvm) {
		fprintf(stderr, "Get GPNVM%d error: invalid GPNVM\n", gpnvm);
		return false;
	}

	// send Get GPNVM command to flash controller
	uint32_t status;
	if (!eefc_send_command(fd, chip, EEFC_FCR_FCMD_GGPB, gpnvm, &status))
		return false;
	if (status & EEFC_FSR_CMDE) {
		fprintf(stderr, "Get GPNVM%d error: command error\n", gpnvm);
		return false;
	}

	uint32_t bits;
	if (!eefc_read_result(fd, chip, &bits))
		return false;
	*value = bits & (1 << gpnvm);

	return true;
}

extern bool eefc_set_gpnvm(int fd, const struct _chip* chip, uint8_t gpnvm)
{
	if (gpnvm >= chip->gpnvm) {
		fprintf(stderr, "Set GPNVM%d error: invalid GPNVM\n", gpnvm);
		return false;
	}

	// send Set GPNVM command to flash controller
	uint32_t status;
	if (!eefc_send_command(fd, chip, EEFC_FCR_FCMD_SGPB, gpnvm, &status))
		return false;
	if (status & EEFC_FSR_CMDE) {
		fprintf(stderr, "Set GPNVM%d error: command error\n", gpnvm);
		return false;
	}
	if (status & EEFC_FSR_FLERR) {
		fprintf(stderr, "Set GPNVM%d error: flash error\n", gpnvm);
		return false;
	}
	return true;
}

extern bool eefc_clear_gpnvm(int fd, const struct _chip* chip, uint8_t gpnvm)
{
	if (gpnvm >= chip->gpnvm) {
		fprintf(stderr, "Clear GPNVM%d error: invalid GPNVM\n", gpnvm);
		return false;
	}

	// send Clear GPNVM command to flash controller
	uint32_t status;
	if (!eefc_send_command(fd, chip, EEFC_FCR_FCMD_CGPB, gpnvm, &status))
		return false;
	if (status & EEFC_FSR_CMDE) {
		fprintf(stderr, "Clear GPNVM%d error: command error\n", gpnvm);
		return false;
	}
	if (status & EEFC_FSR_FLERR) {
		fprintf(stderr, "Clear GPNVM%d error: flash error\n", gpnvm);
		return false;
	}
	return true;
}
