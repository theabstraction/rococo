#include <sexy.types.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <sexy.strings.h>

#ifndef _WIN32
# define _stricmp strcasecmp
# define _strnicmp strncasecmp
#endif

namespace Rococo
{
	bool operator == (const sexstring_key& a, const sexstring_key& b)
	{
		return a.Length == b.Length && memcmp(a.Text, b.Text, a.Length) == 0;
	}

	int32 Compare(const char* a, const char* b) { return strcmp(a, b); }
#ifdef SEXCHAR_IS_WIDE
	int32 Compare(cstr a, cstr b) { return wcscmp(a, b); }
#endif
	int32 CompareI(const char* a, const char* b) { return _stricmp(a, b); }
#ifdef SEXCHAR_IS_WIDE
	int32 CompareI(cstr a, cstr b) { return _wcsicmp(a, b); }
#endif
	int32 CompareI(const char* a, const char* b, int64 count) { return _strnicmp(a, b, count); }
#ifdef SEXCHAR_IS_WIDE
	int32 CompareI(cstr a, cstr b, int64 count) { return _wcsnicmp(a, b, count); }
#endif
	int32 Compare(const char* a, const char* b, int64 count) { return strncmp(a, b, count); }
#ifdef SEXCHAR_IS_WIDE
	int32 Compare(cstr a, cstr b, int64 count) { return wcsncmp(a, b, count); }
#endif
	int32 Compare(sexstring a, const SEXCHAR* b) { return Compare(a->Buffer, b); }
	const char* GetSubString(const char* s, const char *subString) { return strstr(s, subString); }
#ifdef SEXCHAR_IS_WIDE
	cstr GetSubString(cstr s, const rchar *subString) { return wcsstr(s, subString); }
#endif

	void StrNCopy(char* dest, size_t count, const char* src, size_t maxCount)
	{
		strncpy_s(dest, count, src, maxCount);
	}

#ifdef SEXCHAR_IS_WIDE
	errno_t StrNCopy(rchar* dest, size_t count, cstr src, size_t maxCount)
	{
		return wcsncpy_s(dest, count, _In_z_ src, maxCount);
	}
#endif

	bool AreEqual(sexstring a, sexstring b)
	{
		return a->Length == b->Length && Compare(a->Buffer, b->Buffer) == 0;
	}

	bool operator < (const sexstring_key& a, const sexstring_key& b)
	{
		int64 lengthDelta = a.Length - b.Length;
		if (lengthDelta < 0)
		{
			return true;
		}
		else if (lengthDelta > 0)
		{
			return false;
		}
		else
		{
			return Compare(a.Text, b.Text, a.Length) < 0;
		}
	}

	bool operator == (const CStringKey& a, const CStringKey& b)
	{
		return a.hashValue == b.hashValue && AreEqual(a.c_str(), b.c_str());
	}

	bool operator < (const CStringKey& a, const CStringKey& b)
	{
		return a.hashValue < b.hashValue || Compare(a.c_str(), b.c_str()) < 0;
	}
}