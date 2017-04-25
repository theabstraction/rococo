#ifndef Rococo_Strings_H
#define Rococo_Strings_H

#ifdef SEXCHAR_IS_WIDE
#include <wchar.h>
#define SecureFormat swprintf_s // Needs include <wchar.h>. If the output buffer is exhausted it will throw an exception
#define SafeFormat _snwprintf_s // Needs include <wchar.h>. With _TRUNCATE in the MaxCount position, it will truncate buffer overruns, rather than throw
#define SafeVFormat _vsnwprintf_s // Needs include <wchar.h>. With _TRUNCATE in the MaxCount position, it will truncate buffer overruns, rather than throw
#define SecureCopy wcscpy_s // Needs include <wchar.h>. If the output buffer is exhausted it will throw an exception
#define SafeCopy wcsncpy_s // Needs include <wchar.h>. With _TRUNCATE in the MaxCount position, it will truncate buffer overruns, rather than throw
#define SecureCat wcscat_s // Needs include <wchar.h>.  If the output buffer is exhausted it will throw an exception
#define SafeCat wcsncat_s // Needs include <wchar.h>.  With _TRUNCATE in the MaxCount position, it will truncate buffer overruns, rather than throw
#else
#include <stdio.h>
#include <string.h>

#ifndef _TRUNCATE
# define _TRUNCATE ((size_t)-1)
#endif

#define SecureFormat sprintf_s // Needs include <wchar.h>. If the output buffer is exhausted it will throw an exception
#define SafeFormat _snprintf_s // Needs include <wchar.h>. With _TRUNCATE in the MaxCount position, it will truncate buffer overruns, rather than throw
#define SafeVFormat _vsnprintf_s // Needs include <wchar.h>. With _TRUNCATE in the MaxCount position, it will truncate buffer overruns, rather than throw
#define SecureCopy strcpy_s // Needs include <wchar.h>. If the output buffer is exhausted it will throw an exception
#define SafeCopy strncpy_s // Needs include <wchar.h>. With _TRUNCATE in the MaxCount position, it will truncate buffer overruns, rather than throw
#define SecureCat strcat_s // Needs include <wchar.h>.  If the output buffer is exhausted it will throw an exception
#define SafeCat strncat_s // Needs include <wchar.h>.  With _TRUNCATE in the MaxCount position, it will truncate buffer overruns, rather than throw
#endif

namespace Rococo
{
	ROCOCOAPI IStringBuilder
	{
		virtual int AppendFormat(cstr format, ...) = 0;
		virtual operator cstr () const = 0;
		virtual void Free() = 0;
	};

	IStringBuilder* CreateSafeStringBuilder(size_t capacity);

	struct IStringBuffer
	{
		virtual rchar* GetBufferStart() = 0;
		virtual size_t Capacity() const = 0;
	};

	class SafeStackString
	{
	public:
		enum {OPAQUE_CAPACITY = 32 };
		SafeStackString(rchar* _buffer, size_t _capacity) : buffer(_buffer), capacity(_capacity) {}
		rchar* Buffer() { return buffer; }
		size_t Capacity() const { return capacity; }
		void* Data() { return rawData; }
	private:
		rchar* buffer;
		size_t capacity;	
		char rawData[OPAQUE_CAPACITY];
	};

	IStringBuilder* CreateSafeStackStringBuilder(SafeStackString& sss);

   cstr GetFinalNull(cstr s);
   cstr GetRightSubstringAfter(cstr s, rchar c);
   cstr GetFileExtension(cstr s);

   bool Eq(cstr a, cstr b);
}

#endif // Rococo_Strings_H