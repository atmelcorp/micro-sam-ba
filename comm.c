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
#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#if !defined(_MSC_VER)
#include <termios.h>
#include <unistd.h>
#else
#define O_NOCTTY 0
#define O_SYNC 0
#endif

#include "comm.h"
#include "utils.h"


#if defined(_MSC_VER)
#define close(fd) CloseHandle(fd)
#define B115200 CBR_115200

int read(HANDLE fd, void* buf, size_t len)
{
   DWORD num_read = 0;
   BOOL ok = ReadFile(fd, buf, len, &num_read, NULL);

   if (!ok)
      return -1;

   return (int)num_read;
}

int write(HANDLE fd, const void* buf, size_t len)
{
   DWORD num_written = 0;
   BOOL ok = WriteFile(fd, buf, len, &num_written, NULL);

   if (!ok)
      return -1;

   FlushFileBuffers(fd);

   return (int)num_written;
}

void print_windows_error(const char *message)
{
   // Retrieve the system error message for the last-error code

   LPVOID lpMsgBuf;
   DWORD dw = GetLastError();

   FormatMessage(
      FORMAT_MESSAGE_ALLOCATE_BUFFER |
      FORMAT_MESSAGE_FROM_SYSTEM |
      FORMAT_MESSAGE_IGNORE_INSERTS,
      NULL,
      dw,
      MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
      (LPTSTR)&lpMsgBuf,
      0, NULL);

   // Display the error message and exit the process

   fprintf(stderr, message, dw, lpMsgBuf);

   LocalFree(lpMsgBuf);
}

#endif

#if !defined(_MSC_VER)
static bool configure_tty(int fd, int speed)
{
   struct termios tty;

   memset(&tty, 0, sizeof(tty));

   if (tcgetattr(fd, &tty) != 0) {
      perror("error from tcgetattr: ");
      return false;
   }

   if (speed) {
      cfsetospeed(&tty, speed);
      cfsetispeed(&tty, speed);
   }

   tty.c_cflag &= ~(CSIZE | PARENB | PARODD | CSTOPB | CRTSCTS);
   tty.c_cflag |= CS8 | CLOCAL | CREAD;
   tty.c_lflag = 0;
   tty.c_oflag = 0;
   tty.c_cc[VMIN] = 1;
   tty.c_cc[VTIME] = 5;
   tty.c_iflag &= ~(ICRNL | IGNBRK | IXON | IXOFF | IXANY);

   if (tcsetattr(fd, TCSANOW, &tty) != 0) {
      perror("error from tcsetattr: ");
      return false;
   }

   return true;
}
#else
static bool configure_tty(serial_port_handle_t handle, int speed)
{
   DCB dcb;

   memset(&dcb, 0, sizeof(dcb));
   dcb.DCBlength = sizeof(dcb);

   if (!GetCommState(handle, &dcb))
   {
      fprintf(stderr, "Failed to get comm state\r\n");
      return false;
   }

   dcb.BaudRate = speed;
   dcb.fBinary = 1;
   dcb.fParity = false;
   dcb.fOutxCtsFlow = false;
   dcb.fOutxDsrFlow = false;
   dcb.fDtrControl = DTR_CONTROL_DISABLE;
   dcb.fDsrSensitivity = false;
   dcb.fTXContinueOnXoff = false;
   dcb.fOutX = false;
   dcb.fInX = false;
   dcb.fErrorChar = false;
   dcb.fNull = false;
   dcb.fRtsControl = RTS_CONTROL_ENABLE;
   dcb.fAbortOnError = false;
   dcb.ByteSize = 8;
   dcb.Parity = NOPARITY;
   dcb.StopBits = ONESTOPBIT;

   if (!SetCommState(handle, &dcb))
   {
      print_windows_error("Failed to set comm state: (%d) %s\r\n");
      return false;
   }

   return true;
}
#endif

static bool switch_to_binary(serial_port_handle_t fd)
{
   char cmd[] = "N#";
   if (write(fd, cmd, strlen(cmd)) != strlen(cmd))
      return false;
   return read(fd, cmd, 2) == 2;
}

serial_port_handle_t samba_open(const char* device)
{
#if !defined(_MSC_VER)
   int fd = open(device, O_RDWR | O_NOCTTY | O_SYNC);
   if (fd < 0) {
      perror("Could not open device");
      return INVALID_HANDLE_VALUE;
   }
#else
   HANDLE fd = CreateFileA(device, GENERIC_READ | GENERIC_WRITE, 0,
      NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);

   if (fd == INVALID_HANDLE_VALUE)
   {
      perror("Could not open device");
      return INVALID_HANDLE_VALUE;
   }
#endif

   if (!configure_tty(fd, B115200))
   {
      close(fd);
      return INVALID_HANDLE_VALUE;
   }

   if (!switch_to_binary(fd))
   {
      close(fd);
      return INVALID_HANDLE_VALUE;
   }

   return fd;
}

void samba_close(serial_port_handle_t fd)
{
   close(fd);
}

bool samba_read_word(serial_port_handle_t fd, uint32_t addr, uint32_t* value)
{
   char cmd[12];
   snprintf(cmd, sizeof(cmd), "w%08x,#", addr);
   if (write(fd, cmd, strlen(cmd)) != strlen(cmd))
      return false;
   return read(fd, value, 4) == 4;
}

bool samba_write_word(serial_port_handle_t fd, uint32_t addr, uint32_t value)
{
   char cmd[20];
   snprintf(cmd, sizeof(cmd), "W%08x,%08x#", addr, value);
   return write(fd, cmd, strlen(cmd)) == strlen(cmd);
}

bool samba_read(serial_port_handle_t fd, uint8_t* buffer, uint32_t addr, uint32_t size)
{
   char cmd[20];
   while (size > 0) {
      uint32_t count = MIN(size, 1024);
      // workaround for bug when size is exactly 512
      if (count == 512)
         count = 1;
      snprintf(cmd, sizeof(cmd), "R%08x,%08x#", addr, count);
      if (write(fd, cmd, strlen(cmd)) != strlen(cmd))
         return false;
      if (read(fd, buffer, count) != count)
         return false;
      addr += count;
      buffer += count;
      size -= count;
   }
   return true;
}

bool samba_write(serial_port_handle_t fd, uint8_t* buffer, uint32_t addr, uint32_t size)
{
   char cmd[20];
   while (size > 0) {
      uint32_t count = MIN(size, 1024);
      // workaround for bug when size is exactly 512
      if (count == 512)
         count = 1;
      snprintf(cmd, sizeof(cmd), "S%08x,%08x#", addr, count);
      if (write(fd, cmd, strlen(cmd)) != strlen(cmd))
         return false;
      if (write(fd, buffer, count) != count)
         return false;
      buffer += count;
      size -= count;
   }
   return true;
}
