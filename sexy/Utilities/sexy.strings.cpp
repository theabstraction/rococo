#include <sexy.types.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <sexy.strings.h>

#include <sexy.string.keys.inl>

namespace Sexy
{
	bool operator == (const sexstring_key& a, const sexstring_key& b)
	{
		return a.Length == b.Length && memcmp(a.Text, b.Text, a.Length) == 0;
	}

	int32 Compare(const char* a, const char* b) { return strcmp(a, b); }
	int32 Compare(const wchar_t* a, const wchar_t* b) { return wcscmp(a, b); }
	int32 CompareI(const char* a, const char* b) { return _stricmp(a, b); }
	int32 CompareI(const wchar_t* a, const wchar_t* b) { return _wcsicmp(a, b); }
	int32 CompareI(const char* a, const char* b, int count) { return _strnicmp(a, b, count); }
	int32 CompareI(const wchar_t* a, const wchar_t* b, int count) { return _wcsnicmp(a, b, count); }
	int32 Compare(const char* a, const char* b, int count) { return strncmp(a, b, count); }
	int32 Compare(const wchar_t* a, const wchar_t* b, int count) { return wcsncmp(a, b, count); }
	int32 Compare(sexstring a, const SEXCHAR* b) { return Compare(a->Buffer, b); }
	const char* GetSubString(const char* s, const char *subString) { return strstr(s, subString); }
	const wchar_t* GetSubString(const wchar_t* s, const wchar_t *subString) { return wcsstr(s, subString); }

	errno_t StrNCopy(char* dest, size_t count, const char* src, size_t maxCount)
	{
		return strncpy_s(dest, count, _In_z_ src, maxCount);
	}

	errno_t StrNCopy(wchar_t* dest, size_t count, const wchar_t* src, size_t maxCount)
	{
		return wcsncpy_s(dest, count, _In_z_ src, maxCount);
	}

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