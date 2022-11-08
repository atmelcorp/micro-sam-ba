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

#include <string.h>
#include "chipid.h"
#include "comm.h"
#include "utils.h"

static const struct _chip _chips_samx7[] = {
	{ "SAME70Q21A", 0xa1020e00, 0x00000002, 0x400e0c00, 0x00400000, 2048, 9, 0xa5000005 },
	{ "SAME70Q21B", 0xa1020e01, 0x00000002, 0x400e0c00, 0x00400000, 2048, 9, 0xa5000005 },
	{ "SAME70Q20", 0xa1020c00, 0x00000002, 0x400e0c00, 0x00400000, 1024, 9, 0xa5000005 },
	{ "SAME70Q19", 0xa10d0a00, 0x00000002, 0x400e0c00, 0x00400000,  512, 9, 0xa5000005 },
	{ "SAME70N21A", 0xa1020e00, 0x00000001, 0x400e0c00, 0x00400000, 2048, 9, 0xa5000005 },
	{ "SAME70N21B", 0xa1020e01, 0x00000001, 0x400e0c00, 0x00400000, 2048, 9, 0xa5000005 },
	{ "SAME70N20A", 0xa1020c00, 0x00000001, 0x400e0c00, 0x00400000, 1024, 9, 0xa5000005 },
	{ "SAME70N20B", 0xa1020c01, 0x00000001, 0x400e0c00, 0x00400000, 1024, 9, 0xa5000005 },
	{ "SAME70N19", 0xa10d0a00, 0x00000001, 0x400e0c00, 0x00400000,  512, 9, 0xa5000005 },
	{ "SAME70J21", 0xa1020e00, 0x00000000, 0x400e0c00, 0x00400000, 2048, 9, 0xa5000005 },
	{ "SAME70J20", 0xa1020c00, 0x00000000, 0x400e0c00, 0x00400000, 1024, 9, 0xa5000005 },
	{ "SAME70J19", 0xa10d0a00, 0x00000000, 0x400e0c00, 0x00400000,  512, 9, 0xa5000005 },
	{ "SAMS70Q21A", 0xa1120e00, 0x00000002, 0x400e0c00, 0x00400000, 2048, 9, 0xa5000005 },
	{ "SAMS70Q21B", 0xa1120e01, 0x00000002, 0x400e0c00, 0x00400000, 2048, 9, 0xa5000005 },
	{ "SAMS70N21A", 0xa1120e00, 0x00000001, 0x400e0c00, 0x00400000, 2048, 9, 0xa5000005 },
	{ "SAMS70N21B", 0xa1120e01, 0x00000001, 0x400e0c00, 0x00400000, 2048, 9, 0xa5000005 },
	{ "SAMS70Q20", 0xa1120c00, 0x00000002, 0x400e0c00, 0x00400000, 1024, 9, 0xa5000005 },
	{ "SAMS70Q19", 0xa11d0a00, 0x00000002, 0x400e0c00, 0x00400000,  512, 9, 0xa5000005 },
	{ "SAMS70N21A", 0xa1120e00, 0x00000001, 0x400e0c00, 0x00400000, 2048, 9, 0xa5000005 },
	{ "SAMS70N21B", 0xa1120e01, 0x00000001, 0x400e0c00, 0x00400000, 2048, 9, 0xa5000005 },
	{ "SAMS70N20A", 0xa1120c00, 0x00000001, 0x400e0c00, 0x00400000, 1024, 9, 0xa5000005 },
	{ "SAMS70N20B", 0xa1120c01, 0x00000001, 0x400e0c00, 0x00400000, 1024, 9, 0xa5000005 },
	{ "SAMS70N19", 0xa11d0a00, 0x00000001, 0x400e0c00, 0x00400000,  512, 9, 0xa5000005 },
	{ "SAMS70J21", 0xa1120e00, 0x00000000, 0x400e0c00, 0x00400000, 2048, 9, 0xa5000005 },
	{ "SAMS70J20", 0xa1120c00, 0x00000000, 0x400e0c00, 0x00400000, 1024, 9, 0xa5000005 },
	{ "SAMS70J19", 0xa11d0a00, 0x00000000, 0x400e0c00, 0x00400000,  512, 9, 0xa5000005 },
	{ "SAMV71Q21", 0xa1220e00, 0x00000002, 0x400e0c00, 0x00400000, 2048, 9, 0xa5000005 },
	{ "SAMV71Q20", 0xa1220c00, 0x00000002, 0x400e0c00, 0x00400000, 1024, 9, 0xa5000005 },
	{ "SAMV71Q19", 0xa12d0a00, 0x00000002, 0x400e0c00, 0x00400000,  512, 9, 0xa5000005 },
	{ "SAMV71N21", 0xa1220e00, 0x00000001, 0x400e0c00, 0x00400000, 2048, 9, 0xa5000005 },
	{ "SAMV71N20", 0xa1220c00, 0x00000001, 0x400e0c00, 0x00400000, 1024, 9, 0xa5000005 },
	{ "SAMV71N19", 0xa12d0a00, 0x00000001, 0x400e0c00, 0x00400000,  512, 9, 0xa5000005 },
	{ "SAMV71J21", 0xa1220e00, 0x00000000, 0x400e0c00, 0x00400000, 2048, 9, 0xa5000005 },
	{ "SAMV71J20", 0xa1220c00, 0x00000000, 0x400e0c00, 0x00400000, 1024, 9, 0xa5000005 },
	{ "SAMV71J19", 0xa12d0a00, 0x00000000, 0x400e0c00, 0x00400000,  512, 9, 0xa5000005 },
	{ "SAMV70Q20", 0xa1320c00, 0x00000002, 0x400e0c00, 0x00400000, 1024, 9, 0xa5000005 },
	{ "SAMV70Q19", 0xa13d0a00, 0x00000002, 0x400e0c00, 0x00400000,  512, 9, 0xa5000005 },
	{ "SAMV70N20A", 0xa1320c00, 0x00000001, 0x400e0c00, 0x00400000, 1024, 9, 0xa5000005 },
	{ "SAMV70N20B", 0xa1320c01, 0x00000001, 0x400e0c00, 0x00400000, 1024, 9, 0xa5000005 },
	{ "SAMV70N19", 0xa13d0a00, 0x00000001, 0x400e0c00, 0x00400000,  512, 9, 0xa5000005 },
	{ "SAMV70J20", 0xa1320c00, 0x00000000, 0x400e0c00, 0x00400000, 1024, 9, 0xa5000005 },
	{ "SAMV70J19", 0xa13d0a00, 0x00000000, 0x400e0c00, 0x00400000,  512, 9, 0xa5000005 },
};

static const struct _chip _chips_sam4[] = {
	{ "SAM4E8CA" , 0xa3cc0ce0, 0x00120209, 0x400e0a00, 0x00400000,  512,	2, 0xa5000005 },
	{ "SAM4E8CB" , 0xa3cc0ce1, 0x00120209, 0x400e0a00, 0x00400000,  512,	2, 0xa5000005 },
	{ "SAM4E8EA" , 0xa3cc0ce0, 0x00120208, 0x400e0a00, 0x00400000,  512,	2, 0xa5000005 },
	{ "SAM4E8EB" , 0xa3cc0ce1, 0x00120208, 0x400e0a00, 0x00400000,  512,	2, 0xa5000005 },
};

static const struct _chip_serie _chip_series[] = {
	{
		.name       = "samx7",
		.cidr_reg   = 0x400e0940,
		.exid_reg   = 0x400e0944,
		.rstccr_reg = 0x400e1800,
		.nb_chips   = ARRAY_SIZE(_chips_samx7),
		.chips      = _chips_samx7,
	},
	{
		.name       = "sam4",
		.cidr_reg   = 0x400e0740,
		.exid_reg   = 0x400e0744,
		.rstccr_reg = 0x400e1800,
		.nb_chips   = ARRAY_SIZE(_chips_sam4),
		.chips      = _chips_sam4,
	},
};

const struct _chip_serie* chipid_get_serie(const char* name)
{
	for (int i = 0; i < ARRAY_SIZE(_chip_series); i++)
		if (!strcmp(_chip_series[i].name, name))
			return &_chip_series[i];
	return NULL;
}

bool chipid_check_serie(serial_port_handle_t fd, const struct _chip_serie* serie, const struct _chip** chip)
{
	// Read chip identifiers (CIDR/EXID)
	uint32_t cidr, exid;
	if (!samba_read_word(fd, serie->cidr_reg, &cidr))
		return false;
	if (!samba_read_word(fd, serie->exid_reg, &exid))
		return false;

	printf("Testing series %s\nRead CIDR %08x EXID %08x\n", serie->name, cidr, exid);

	// Identify chip and read its flash infos
	for (int i = 0; i < serie->nb_chips; i++) {
		if (serie->chips[i].cidr == cidr && serie->chips[i].exid == exid) {
			*chip = &serie->chips[i];
			return true;
		}
	}

	return false;
}

const struct _chip_serie* chipid_identity_serie(serial_port_handle_t fd, const struct _chip** chip)
{
	for (int i = 0; i < ARRAY_SIZE(_chip_series); i++)
		if (chipid_check_serie(fd, &_chip_series[i], chip))
			return &_chip_series[i];
	return NULL;
}
