#ifndef Rococo_Strings_H
#define Rococo_Strings_H

#ifdef SEXCHAR_IS_WIDE
#error "Wide characters no longer supported."
#else
#include <stdio.h>
#include <string.h>

#ifndef _TRUNCATE
# define _TRUNCATE ((size_t)-1)
#endif

#define SecureFormat sprintf_s // Needs include <wchar.h>. If the output buffer is exhausted it will throw an exception
#define SafeFormat _snprintf_s // Needs include <wchar.h>. With _TRUNCATE in the MaxCount position, it will truncate buffer overruns, rather than throw
#define SafeVFormat _vsnprintf_s // Needs include <wchar.h>. With _TRUNCATE in the MaxCount position, it will truncate buffer overruns, rather than throw
#endif

namespace Rococo
{
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

#endif // Rococo_Strings_H