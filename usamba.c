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

#include <sys/types.h>
#include <sys/stat.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#if !defined(_MSC_VER)
#include <unistd.h>
#endif
#include "chipid.h"
#include "comm.h"
#include "eefc.h"
#include "utils.h"

//#define BUFFER_SIZE 8192
#define BUFFER_SIZE 256

static bool get_file_size(const char* filename, uint32_t* size)
{
	struct stat st;

	if (stat(filename, &st) != 0) {
		fprintf(stderr, "Could not access '%s'\n", filename);
		return false;
	}

	*size = st.st_size;
	return true;
}

static bool read_flash(serial_port_handle_t fd, const struct _chip* chip, uint32_t addr, uint32_t size, const char* filename)
{
	FILE* file = fopen(filename, "wb");
	if (!file) {
		fprintf(stderr, "Could not open '%s' for writing\n", filename);
		return false;
	}

	uint8_t buffer[BUFFER_SIZE];
	uint32_t total = 0;
	while (total < size) {
		uint32_t count = MIN(BUFFER_SIZE, size - total);
		if (!eefc_read(fd, chip, buffer, addr, count)) {
			fclose(file);
			return false;
		}

		if (fwrite(buffer, 1, count, file) != count) {
			fprintf(stderr, "Error while writing to '%s'", filename);
			fclose(file);
			return false;
		}

		total += count;
		addr += count;
	}

	fclose(file);
	return true;
}

static bool write_flash(serial_port_handle_t fd, const struct _chip* chip, const char* filename, uint32_t addr, uint32_t size)
{
	FILE* file = fopen(filename, "rb");
	if (!file) {
		fprintf(stderr, "Could not open '%s' for reading\n", filename);
		return false;
	}

	uint8_t buffer[BUFFER_SIZE];
	uint32_t total = 0;
	while (total < size) {
		uint32_t count = MIN(BUFFER_SIZE, size - total);
		if (fread(buffer, 1, count, file) != count) {
			fprintf(stderr, "Error while reading from '%s'", filename);
			fclose(file);
			return false;
		}

		printf("Writing flash address 0x%08X to 0x%08X\r", addr, addr + count);
		if (!eefc_write(fd, chip, buffer, addr, count)) {
			fclose(file);
			return false;
		}

		total += count;
		addr += count;
	}

	fclose(file);
	return true;
}

static bool verify_flash(serial_port_handle_t fd, const struct _chip* chip, const char* filename, uint32_t addr, uint32_t size)
{
	FILE* file = fopen(filename, "rb");
	if (!file) {
		fprintf(stderr, "Could not open '%s' for reading\n", filename);
		return false;
	}

	uint8_t buffer1[BUFFER_SIZE];
	uint8_t buffer2[BUFFER_SIZE];
	uint32_t total = 0;
	while (total < size) {
		uint32_t count = MIN(BUFFER_SIZE, size - total);

		printf("Reading file offset %d to %d\r", total, total + count);

		if (fread(buffer1, 1, count, file) != count) {
			fprintf(stderr, "Error while reading from '%s'", filename);
			fclose(file);
			return false;
		}

		printf("Reading flash address 0x%08X to 0x%08X\r", addr, addr + count);

		if (!eefc_read(fd, chip, buffer2, addr, count)) {
			fclose(file);
			return false;
		}

		for (int i = 0; i < BUFFER_SIZE; i++) {
			if (buffer1[i] != buffer2[i]) {
				fprintf(stderr, "\r\n** Verify failed, first difference at offset %d **\r\n\r\n", addr + i);
				fclose(file);
				return false;
			}

			if ((i % 256) == 0)
			{
				printf("Checking offset %d                                  \r", total);
			}
		}

		total += count;
		addr += count;
	}

	printf("                              \r");

	fclose(file);
	return true;
}

static void usage(char* prog)
{
	printf("Usage: %s <port> (read|write|verify|erase-all|gpnvm) [args]*\n", prog);
	printf("\n");
	printf("- Reading Flash:\n");
	printf("    %s <port> read <filename> <start-address> <size>\n", prog);
	printf("\n");
	printf("- Writing Flash:\n");
	printf("    %s <port> write <filename> <start-address>\n", prog);
	printf("\n");
	printf("- Verify Flash:\n");
	printf("    %s <port> verify <filename> <start-address>\n", prog);
	printf("\n");
	printf("- Erasing Flash:\n");
	printf("    %s <port> erase-all\n", prog);
	printf("\n");
	printf("- Getting/Setting/Clearing GPNVM:\n");
	printf("    %s <port> gpnvm (get|set|clear) <gpnvm_number>\n", prog);
	printf("\n");
	printf("for all commands:\n");
	printf("    <port> is the USB device node for the SAM-BA bootloader, for\n");
	printf("         example '/dev/ttyACM0'\n");
	printf("    <start-addres> and <size> can be specified in decimal, hexadecimal (if\n");
	printf("         prefixed by '0x') or octal (if prefixed by 0).\n");
}

enum {
	CMD_READ = 1,
	CMD_WRITE = 2,
	CMD_VERIFY = 3,
	CMD_ERASE_ALL = 4,
	CMD_GPNVM_GET = 5,
	CMD_GPNVM_SET = 6,
	CMD_GPNVM_CLEAR = 7,
};

int main(int argc, char *argv[])
{
	serial_port_handle_t fd = INVALID_HANDLE_VALUE;
	int command = 0;
	char* port = NULL;
	char* filename = NULL;
	uint32_t addr = 0;
	uint32_t size = 0;
	bool err = true;

	// parse command line
	if (argc < 3) {
		fprintf(stderr, "Error: not enough arguments\n");
		usage(argv[0]);
		return EXIT_FAILURE;
	}
	port = argv[1];
	char* cmd_text = argv[2];
	if (!strcmp(cmd_text, "read")) {
		if (argc == 6) {
			command = CMD_READ;
			filename = argv[3];
			addr = strtol(argv[4], NULL, 0);
			size = strtol(argv[5], NULL, 0);
			err = false;
		} else {
			fprintf(stderr, "Error: invalid number of arguments\n");
		}
	} else if (!strcmp(cmd_text, "write")) {
		if (argc == 5) {
			command = CMD_WRITE;
			filename = argv[3];
			addr = strtol(argv[4], NULL, 0);
			err = false;
		} else {
			fprintf(stderr, "Error: invalid number of arguments\n");
		}
	} else if (!strcmp(cmd_text, "verify")) {
		if (argc == 5) {
			command = CMD_VERIFY;
			filename = argv[3];
			addr = strtol(argv[4], NULL, 0);
			err = false;
		} else {
			fprintf(stderr, "Error: invalid number of arguments\n");
		}
	} else if (!strcmp(cmd_text, "erase-all")) {
		if (argc == 3) {
			command = CMD_ERASE_ALL;
			err = false;
		} else {
			fprintf(stderr, "Error: invalid number of arguments\n");
		}
	} else if (!strcmp(cmd_text, "gpnvm")) {
		if (argc == 5) {
			if (!strcmp(argv[3], "get")) {
				command = CMD_GPNVM_GET;
				addr = strtol(argv[4], NULL, 0);
				err = false;
			} else if (!strcmp(argv[3], "set")) {
				command = CMD_GPNVM_SET;
				addr = strtol(argv[4], NULL, 0);
				err = false;
			} else if (!strcmp(argv[3], "clear")) {
				command = CMD_GPNVM_CLEAR;
				addr = strtol(argv[4], NULL, 0);
				err = false;
			} else {
				fprintf(stderr, "Error: unknown GPNVM command '%s'\n", argv[3]);
			}
		} else {
			fprintf(stderr, "Error: invalid number of arguments\n");
		}
	} else {
		fprintf(stderr, "Error: unknown command '%s'\n", cmd_text);
	}
	if (err) {
		usage(argv[0]);
		return EXIT_FAILURE;
	}

	err = true;

	printf("Port: %s\n", port);
	fd = samba_open(port);
	if (fd == INVALID_HANDLE_VALUE)
		return EXIT_FAILURE;

	// Identify chip
	const struct _chip* chip;
	if (!chipid_identity_serie(fd, &chip)) {
		fprintf(stderr, "Could not identify chip\n");
		goto exit;
	}
	printf("Device: Atmel %s\n", chip->name);

	// Read and check flash information
	struct _eefc_locks locks;
	if (!eefc_read_flash_info(fd, chip, &locks)) {
		fprintf(stderr, "Could not read flash information\n");
		goto exit;
	}
	printf("Flash Size: %uKB\n", chip->flash_size);

	// Execute command
	switch (command) {
		case CMD_READ:
		{
			printf("Reading %d bytes at 0x%08x to file '%s'\n", size, addr, filename);
			if (read_flash(fd, chip, addr, size, filename)) {
				err = false;
			}
			break;
		}

		case CMD_WRITE:
		{
			if (get_file_size(filename, &size)) {
				printf("Unlocking %d bytes at 0x%08x\n", size, addr);
				if (eefc_unlock(fd, chip, &locks, addr, size)) {
					printf("Writing %d bytes at 0x%08x from file '%s'\n", size, addr, filename);
					if (write_flash(fd, chip, filename, addr, size)) {
						err = false;
					}
				}
			}
			break;
		}

		case CMD_VERIFY:
		{
			if (get_file_size(filename, &size)) {
				printf("Verifying %d bytes at 0x%08x with file '%s' (0x%X)\n", size, addr, filename, size);
				if (verify_flash(fd, chip, filename, addr, size)) {
					err = false;
				}
			}
			break;
		}

		case CMD_ERASE_ALL:
		{
			printf("Unlocking all pages\n");
			if (eefc_unlock(fd, chip, &locks, 0, chip->flash_size * 1024)) {
				printf("Erasing all pages\n");
				if (eefc_erase_all(fd, chip)) {
					err = false;
				}
			}
			break;
		}

		case CMD_GPNVM_GET:
		{
			printf("Getting GPNVM%d\n", addr);
			bool value;
			if (eefc_get_gpnvm(fd, chip, addr, &value)) {
				printf("GPNVM%d is %s\n", addr, value ? "set" : "clear");
				err = false;
			}
			break;
		}

		case CMD_GPNVM_SET:
		{
			if (!addr && !getenv("GPNVM0_CONFIRM")) {
				fprintf(stderr, "To avoid setting the security bit (GPNVM0) by mistake, an additional\n");
				fprintf(stderr, "confirmation is required: please add a 'GPNVM0_CONFIRM' environment\n");
				fprintf(stderr, "variable with any value and try again.\n");
			} else {
				printf("Setting GPNVM%d\n", addr);
				if (eefc_set_gpnvm(fd, chip, addr)) {
					err = false;
				}
			}
			break;
		}

		case CMD_GPNVM_CLEAR:
		{
			printf("Clearing GPNVM%d\n", addr);
			if (eefc_clear_gpnvm(fd, chip, addr)) {
				err = false;
			}
			break;
		}
	}

exit:
	fflush(stdout);
	samba_close(fd);
	if (err) {
		fprintf(stderr, "Operation failed\n");
		return EXIT_FAILURE;
	} else {
		return EXIT_SUCCESS;
	}
}
