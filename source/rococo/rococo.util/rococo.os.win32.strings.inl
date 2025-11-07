#pragma once

// Windows has its own idiosyncratic wchar_t string library, thus we put the wchar_t string files here rather than rococo.strings.cpp
namespace Rococo::Strings
{
#ifdef _DEBUG
	ROCOCO_API int PrintD(const char* format, ...)
	{
		char message[2048];

		va_list args;
		va_start(args, format);
		int len = SafeVFormat(message, sizeof message, format, args);

		OutputDebugStringA(message);

		return len;

	}
#endif

	ROCOCO_API int SafeVFormat(ROCOCO_WIDECHAR* buffer, size_t capacity, crwstr format, va_list args)
	{
		int count = _vsnwprintf_s(buffer, capacity, capacity, format, args);

		if (count >= capacity)
		{
			return -1;
		}

		return count;
	}

	ROCOCO_API int SafeVFormat(char* buffer, size_t capacity, const char* format, va_list args)
	{
		int count = vsnprintf(buffer, capacity, format, args);
		if (count >= capacity)
		{
			return -1;
		}

		return count;
	}

	ROCOCO_API bool EndsWith(crwstr bigString, crwstr suffix)
	{
		size_t len = wcslen(suffix);
		size_t lenBig = wcslen(bigString);
		crwstr t = bigString + lenBig - len;
		return Eq(suffix, t);
	}

	ROCOCO_API bool Eq(crwstr a, crwstr b)
	{
		return wcscmp(a, b) == 0;
	}

	ROCOCO_API bool EqI(crwstr a, crwstr b)
	{
		return _wcsicmp(a, b) == 0;
	}

	ROCOCO_API bool StartsWith(crwstr bigString, crwstr prefix)
	{
		return wcsncmp(bigString, prefix, wcslen(prefix)) == 0;
	}

	ROCOCO_API bool EqI(const char* a, const char* b)
	{
		return _strcmpi(a, b) == 0;
	}

	// N.B sexy script language string length is int32 with max 2^31-1 chars
	ROCOCO_API int32 StringLength(const wchar_t* s)
	{
		enum { MAX_INT32 = 0x7FFFFFFF };
		size_t l = wcslen(s);
		if (l > MAX_INT32)
		{
			Throw(0, "The string length exceeded INT_MAX characters");
		}

		return (int32)l;
	}
}
