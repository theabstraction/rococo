#pragma once
#include <HAL/Platform.h>

#ifdef _WIN32
# pragma warning(disable: 4456)
# pragma warning(disable: 4459)
# pragma warning(disable: 4996)

// 4661 allows setjmp to compile. I have gone at lengths to make sure it does not cause issues with C++ where it is used in libJPG and I see no evidence of vulnerability
# pragma warning(disable: 4611)
#else
#endif

#ifndef _WIN32
#include <stdarg.h>
enum { _TRUNCATE = -1 };
int _vsnprintf_s(char* message, size_t capacity, size_t nBytesToWrite, const char* format, va_list args);
#endif
