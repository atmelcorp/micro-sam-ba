Micro SAM-BA
------------

# Description

This simple tool can be used to program the internal flash of the following
microcontrollers:

  - Atmel SAME70/S70/V70/V71

Communication with SAM-BA bootloader is only supported through USB.

Before using the tool, the MCU must be put in "Boot from ROM" mode, either by
fully erasing it using the ERASE pin or by clearing GPVNM1 (see datasheet for
more information).  When the device is in this mode, it will be enumerated as:

    03eb:6124 Atmel Corp. at91sam SAMBA bootloader

# Usage

Usage: ``./usamba <port> (read|write|verify|erase-all|gpnvm) [args]*``

- Read Flash:
    ``./usamba <port> read <filename> <start-address> <size>``

- Write Flash:
    ``./usamba <port> write <filename> <start-address>``

- Verify Flash:
    ``./usamba <port> verify <filename> <start-address>``

- Erase Flash:
    ``./usamba <port> erase-all``

- Get/Set/Clear GPNVM:
    ``./usamba <port> gpnvm (get|set|clear) <gpnvm_number>``

for all commands:
    ``<port>`` is the USB device node for the SAM-BA bootloader, for
         example ``/dev/ttyACM0``.
    ``<start-address>`` and ``<size>`` can be specified in decimal, hexadecimal (if
         prefixed by ``0x``) or octal (if prefixed by ``0``).
