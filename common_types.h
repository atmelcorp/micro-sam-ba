#pragma once

#if defined(_MSC_VER)
#define WINDOWS_LEAN_AND_MEAN
#include <windows.h>
#endif

#if defined(_MSC_VER)
typedef HANDLE serial_port_handle_t;
#else
typedef int serial_port_handle_t;
#define INVALID_HANDLE_VALUE -1
#endif

