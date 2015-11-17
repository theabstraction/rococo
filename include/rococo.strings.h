#ifndef Rococo_Strings_H
#define Rococo_Strings_H

#include <wchar.h>

#define SecureFormat swprintf_s // Needs include <wchar.h>. If the output buffer is exhausted it will throw an exception
#define SafeFormat _snwprintf_s // Needs include <wchar.h>. With _TRUNCATE in the MaxCount position, it will truncate buffer overruns, rather than throw
#define SafeVFormat _vsnwprintf_s // Needs include <wchar.h>. With _TRUNCATE in the MaxCount position, it will truncate buffer overruns, rather than throw
#define SecureCopy wcscpy_s // Needs include <wchar.h>. If the output buffer is exhausted it will throw an exception
#define SafeCopy wcsncpy_s // Needs include <wchar.h>. With _TRUNCATE in the MaxCount position, it will truncate buffer overruns, rather than throw
#define SecureCat wcscat_s // Needs include <wchar.h>.  If the output buffer is exhausted it will throw an exception
#define SafeCat wcsncat_s // Needs include <wchar.h>.  With _TRUNCATE in the MaxCount position, it will truncate buffer overruns, rather than throw

namespace Rococo
{
	ROCOCOAPI IStringBuilder
	{
		virtual int AppendFormat(const wchar_t* format, ...) = 0;
		virtual operator const wchar_t* () const = 0;
		virtual void Free() = 0;
	};

	IStringBuilder* CreateSafeStringBuilder(size_t capacity);

	struct IStringBuffer
	{
		virtual wchar_t* GetBufferStart() = 0;
		virtual size_t Capacity() const = 0;
	};

	class SafeStackString
	{
	public:
		enum {OPAQUE_CAPACITY = 32 };
		SafeStackString(wchar_t* _buffer, size_t _capacity) : buffer(_buffer), capacity(_capacity) {}
		wchar_t* Buffer() { return buffer; }
		size_t Capacity() const { return capacity; }
		void* Data() { return rawData; }
	private:
		wchar_t* buffer;
		size_t capacity;	
		char rawData[OPAQUE_CAPACITY];
	};

	IStringBuilder* CreateSafeStackStringBuilder(SafeStackString& sss);
}

#endif  Rococo_Strings_H