#pragma once

#ifdef SEXCHAR_IS_WIDE
# error "Wide characters no longer supported."
#endif

#ifdef ROCOCO_USE_SAFE_V_FORMAT
# include <stdarg.h>
#endif

namespace Rococo
{
   int SecureFormat(char* buffer, size_t capacity, const char* format, ...);
   int SafeFormat(char* buffer, size_t capacity, const char* format, ...);

#ifdef ROCOCO_USE_SAFE_V_FORMAT
   int SafeVFormat(char* buffer, size_t capacity, const char* format, va_list args);

# ifndef _WIN32
   int sscanf_s(const char* buffer, const char* format, ...);
# endif
#endif

	struct IStringBuffer
	{
		virtual rchar* GetBufferStart() = 0;
		virtual size_t Capacity() const = 0;
	};

   cstr GetFinalNull(cstr s);
   cstr GetRightSubstringAfter(cstr s, rchar c);
   cstr GetFileExtension(cstr s);

   bool Eq(cstr a, cstr b);
}
