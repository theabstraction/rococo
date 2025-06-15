#pragma once
#include "rococo.jpg.prelude.h"

#ifndef _WIN32

#include <stdarg.h>
enum {_TRUNCATE = -1 };
int _vsnprintf_s(char* message, size_t capacity, size_t nBytesToWrite, cstr format, va_list args);
#endif