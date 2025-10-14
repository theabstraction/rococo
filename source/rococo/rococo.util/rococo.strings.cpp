// Copyright (c)2025 Mark Anthony Taylor. Email: mark.anthony.taylor@gmail.com. All rights reserved.
#include <rococo.types.h>

#define _CRT_NONSTDC_NO_WARNINGS

#ifdef ROCOCO_WIDECHAR_IS_CHAR_16_T
#include "rococo.char16.inl"
#endif

#include <stdarg.h>
#include <rococo.functional.h>
#include <rococo.debugging.h>

#ifdef _WIN32
# include <malloc.h>
#else 
# include <alloca.h>
# define strtok_s strtok_r
#endif

#include <vector>
#include <rococo.io.h>

#define ROCOCO_USE_SAFE_V_FORMAT
#include <rococo.strings.h>
#include <rococo.reflector.h>

#ifdef __APPLE__
# define _stricmp strcasecmp
#endif

using namespace Rococo;
using namespace Rococo::Strings;

#ifndef ROCOCO_API
# define ROCOCO_API __declspec(dllexport)
#endif

namespace Rococo
{
	ROCOCO_API bool DoesModifiedFilenameMatchResourceName(cstr modifiedFilename, cstr resourceName)
	{
		cstr p = modifiedFilename;
		cstr q = resourceName + 1;

		while (*p != 0)
		{
			if (*p != *q)
			{
				if (*p == '\\' && *q == '/')
				{
					// ok
				}
				else
				{
					return false;
				}
			}

			p++;
			q++;
		}

		return *q == 0;
	}
}

namespace StringsAnon
{
	class ExpandingBuffer : public IExpandingBuffer
	{
		std::vector<uint8> internalBuffer;
	public:
		ExpandingBuffer(size_t capacity) : internalBuffer(capacity)
		{
		}

		virtual ~ExpandingBuffer()
		{

		}

		const uint8* GetData() const override
		{
			if (internalBuffer.capacity() == 0) return nullptr;
			else return &internalBuffer[0];
		}

		uint8* GetData() override
		{
			if (internalBuffer.capacity() == 0) return nullptr;
			else return &internalBuffer[0];
		}

		size_t Length() const override
		{
			return internalBuffer.size();
		}

		void Resize(size_t length) override
		{
			internalBuffer.resize(length);
		}

		void Free() override
		{
			delete this;
		}
	};

	struct DynamicStringBuilder : StringBuilder, public IDynamicStringBuilder
	{
		std::vector<char> internalBuffer;

		DynamicStringBuilder(size_t initCapacity)
		{
			internalBuffer.resize(initCapacity);
			Clear();
		}

		virtual ~DynamicStringBuilder()
		{

		}

		StringBuilder& Builder() override
		{
			return *this;
		}

		StringBuilder& AppendFormat(const char* format, ...) override
		{
			size_t startSize = internalBuffer.size()-1;

			internalBuffer.pop_back(); // Get rid of the zero

			bool isDone = false;

			for (size_t sublen = 256; sublen <= 128_megabytes; sublen *= 2)
			{
				internalBuffer.resize(startSize + sublen);

				va_list args;
				va_start(args, format);
				int nChars = SafeVFormat(internalBuffer.data() + startSize, sublen, format, args);

				if (nChars < 0 || nChars >= (sublen - 1))
				{
					// overflow, so retry
					continue;
				}
				else
				{
					internalBuffer.resize(startSize + nChars);
					internalBuffer.push_back(0);
					isDone = true;
					break;
				}
			}

			if (!isDone)
			{
				Throw(0, "Cannot append formatted string. It was in excess of 128 megabytes");
			}

			return *this;
		}

		StringBuilder& operator << (cstr text) override
		{
			size_t len = strlen(text);
			size_t containerLen = internalBuffer.size();

			internalBuffer.resize(containerLen + len);
			memcpy(internalBuffer.data() + containerLen - 1, text, len);
			internalBuffer[containerLen + len - 1] = 0;

			return *this;
		}

		StringBuilder& AppendChar(char c) override
		{
			char data[2];
			data[0] = c;
			data[1] = 0;
			return *this << data;
		}

		StringBuilder& operator << (int32 value) override
		{
			return AppendFormat("%d", value);
		}

		StringBuilder& operator << (uint32 value) override
		{
			return AppendFormat("%u", value);
		}

		StringBuilder& operator << (int64 value) override
		{
			return AppendFormat("%lld", value);
		}

		StringBuilder& operator << (uint64 value)  override
		{
			return AppendFormat("%llu", value);
		}

		StringBuilder& operator << (float value) override
		{
			return AppendFormat("%f", value);
		}

		StringBuilder& operator << (double value)  override
		{
			return AppendFormat("%lf", value);
		}

		fstring operator * () const override
		{
			if (internalBuffer.size() >= 0x80000000LL) Throw(0, "DynamicStringBuilder: Cannot convert to fstring. Buffer too large ");
			return fstring{ internalBuffer.data(), Length() };
		}

		void Clear() override
		{
			internalBuffer.resize(0);
			internalBuffer.push_back(0);
		}

		int32 Length() const override
		{
			return (int32)(internalBuffer.size()-1);
		}

		void Free() override
		{
			delete this;
		}
	};
}

namespace Rococo::Strings
{
	char ToHex(int32 c)
	{
		static const char* const digits = "01234567890ABCDEF";
		return digits[c & 0xF];
	}

	ROCOCO_API void AppendAsciiCode(StringBuilder& sb, char c)
	{
		char buf[3] = { 0,0,0 };
		buf[0] = ToHex(c >> 16);
		buf[1] = ToHex(c);
		sb << buf;
	}

	ROCOCO_API void AppendEscapedSexyString(StringBuilder& sb, cstr text)
	{
		for (const char* s = text; *s != 0; s++)
		{
			char c = *s;
			switch (c)
			{
			case '\a': sb << "&a"; break;
			case '\b': sb << "&b"; break;
			case '\f': sb << "&f"; break;
			case '\r': sb << "&r"; break;
			case '\n': sb << "&n"; break;
			case '\t': sb << "&t"; break;
			default:
				if (c < 31 || c > 127)
				{
					sb << "&x";
					AppendAsciiCode(sb, c);
				}
				else
				{
					char buf[2] = { c,0 };
					sb << buf;
				}
				break;
			}
		}
	}

#ifndef _WIN32

	void Format(OUT HString& target, cstr format, ...)
	{
		thread_local std::vector<char> buffer;

		if (buffer.size() < 64)
		{
			buffer.resize(64);
		}

		va_list args;
		va_start(args, format);

		for (;;)
		{
			int count = vsnprintf(buffer.data(), buffer.size(), format, args);
			if (count >= buffer.size() - 1)
			{
				// Truncated, so double the buffer size and try again
				buffer.resize(buffer.size() * 2ULL);
				continue;
			}
			else if (count < 0)
			{
				Throw(0, "vsnprintf_s failed");
			}
			else
			{
				// No truncation
				target = buffer.data();
				return;
			}
		}
	}
#else
	void Format(OUT HString& target, cstr format, ...)
	{
		thread_local std::vector<char> buffer;

		if (buffer.size() == 0)
		{
			buffer.resize(64);
		}

		va_list args;
		va_start(args, format);
		
		for (;;)
		{
			_set_errno(0);
			int count = vsnprintf_s(buffer.data(), buffer.size(), _TRUNCATE, format, args);
			if (count == -1)
			{
				int err;
				_get_errno(&err);
				if (err != 0)
				{
					char errBuf[256];
					strerror_s(errBuf, err);
					Throw(0, "%s returned an error code %d. %s", __ROCOCO_FUNCTION__, err, errBuf);
				}
				else // Truncated, so double the buffer size and try again
				{
					buffer.resize(buffer.size() * 2ULL);
				}
			}
			else
			{
				// No truncation
				target = buffer.data();
				return;
			}
		}
	}
#endif
	ROCOCO_API int32 MakePath(U8FilePath& combinedPath, cstr rootPath, cstr subdirectory)
	{
		auto len = strlen(rootPath);
		if (len > 0)
		{
			if (rootPath[len - 1] != IO::GetFileSeparator())
			{
				return SecureFormat(combinedPath.buf, "%s%s%s", rootPath, IO::GetFileSeparatorString(), subdirectory);
			}
			else
			{
				return SecureFormat(combinedPath.buf, "%s%s", rootPath, subdirectory);
			} 
		}
		else
		{
			return SecureFormat(combinedPath.buf, "%s%s", IO::GetFileSeparatorString(), subdirectory);
		}
	}

	ROCOCO_API int32 Format(U8FilePath& path, cstr format, ...)
	{
		va_list args;
		va_start(args, format);
		int count = SafeVFormat(path.buf, path.CAPACITY, format, args);
		if (count == -1)
		{
			Throw(0, "%s failed. Buffer length exceeded. Format String: %s", __ROCOCO_FUNCTION__, format);
		}
		return count;
	}

	ROCOCO_API int32 Format(WideFilePath& path, crwstr format, ...)
	{
		va_list args;
		va_start(args, format);
		int count = SafeVFormat(path.buf, path.CAPACITY, format, args);
		if (count == -1)
		{
			Throw(0, "%s failed. Buffer length exceeded. Format String: %ls", __ROCOCO_FUNCTION__, format);
		}
		return count;
	}

	ROCOCO_API cstr FindSubstring(cr_substring bigText, const fstring& searchTerm)
	{
		if (bigText.Length() < searchTerm.length)
		{
			return nullptr;
		}

		cstr end = bigText.finish - searchTerm.length;

		for (cstr s = bigText.start; s <= end; s++)
		{
			if (memcmp(s, searchTerm, searchTerm.length) == 0)
			{
				return s;
			}
		}

		return nullptr;
	}

	ROCOCO_API cstr FindSubstring(cr_substring bigText, cr_substring searchTerm)
	{
		if (bigText.Length() < searchTerm.Length())
		{
			return nullptr;
		}

		cstr end = bigText.finish - searchTerm.Length();

		for (cstr s = bigText.start; s <= end; s++)
		{
			if (memcmp(s, searchTerm.start, searchTerm.Length()) == 0)
			{
				return s;
			}
		}

		return nullptr;
	}

	ROCOCO_API int ForEachOccurence(cr_substring text, cr_substring searchTerm, Rococo::Function<void(cr_substring match)> lambda)
	{
		int count = 0;

		Substring specimen = text;
		for (;;)
		{
			cstr nextOccurence = FindSubstring(specimen, searchTerm);
			if (!nextOccurence)
			{
				break;
			}

			count++;

			Substring result{ nextOccurence, nextOccurence + searchTerm.Length() };
			lambda(result);

			specimen = { result.finish, specimen.finish };
		}

		return count;
	}

	ROCOCO_API cstr FindSubstring(cstr bigText, cstr searchTerm)
	{
		return strstr(bigText, searchTerm);
	}

#ifdef ROCOCO_WIDECHAR_IS_WCHAR_T
	ROCOCO_API crwstr FindSubstring(const wchar_t* bigText, crwstr searchTerm)
	{
		return wcsstr(bigText, searchTerm);
	}
#endif

	ROCOCO_API cstr ForwardFind(char c, cr_substring text)
	{
		if (text.empty()) return nullptr;

		for (cstr p = text.start; p != text.finish; p++)
		{
			if (*p == c)
			{
				return p;
			}
		}

		return nullptr;
	}

	ROCOCO_API bool IsCapital(char c)
	{
		return c >= 'A' && c <= 'Z';
	}

	ROCOCO_API bool IsLowerCase(char c)
	{
		return c >= 'a' && c <= 'z';
	}

	ROCOCO_API bool IsAlphabetical(char c)
	{
		return IsCapital(c) || IsLowerCase(c);
	}

	ROCOCO_API bool IsNumeric(char c)
	{
		return c >= '0' && c <= '9';
	}

	ROCOCO_API bool IsAlphaNumeric(char c)
	{
		return IsAlphabetical(c) || IsNumeric(c);
	}

	ROCOCO_API bool IsAlphaNumeric(cr_substring s)
	{
		for (auto c : s)
		{
			if (!IsAlphaNumeric(c))
			{
				return false;
			}
		}

		return true;
	}

	ROCOCO_API int32 HashArg(int32 x)
	{
		struct ANON
		{
			static int robert_jenkins_32bit_hash(int32 key)
			{
				key = ~key + (key << 15); // key = (key << 15) - key - 1;
				key = key ^ (key >> 12);
				key = key + (key << 2);
				key = key ^ (key >> 4);
				key = key * 2057; // key = (key + (key << 3)) + (key << 11);
				key = key ^ (key >> 16);
				return key;
			}
		};

		return ANON::robert_jenkins_32bit_hash(x);
	}

	ROCOCO_API int32 HashArg(int64 x)
	{
		struct ANON
		{
			static int robert_jenkins_64bit_hash(int64 key)
			{
				key = (~key) + (key << 18); // key = (key << 18) - key - 1;
				key = key ^ (key >> 31);
				key = key * 21; // key = (key + (key << 2)) + (key << 4);
				key = key ^ (key >> 11);
				key = key + (key << 6);
				key = key ^ (key >> 22);
				return (int)key;
			}
		};

		return ANON::robert_jenkins_64bit_hash(x);
	}

	ROCOCO_API size_t HashArg(cstr s)
	{
		struct ANON
		{
			static size_t jenkins_one_at_a_time_hash(cstr s, size_t len)
			{
				size_t hash, i;
				for (hash = i = 0; i < len; ++i)
				{
					hash += s[i];
					hash += (hash << 10);
					hash ^= (hash >> 6);
				}
				hash += (hash << 3);
				hash ^= (hash >> 11);
				hash += (hash << 15);
				return hash;
			}
		};

		if (s == nullptr) return (size_t) -1LL;
		return ANON::jenkins_one_at_a_time_hash(s, StringLength(s));
	}

	ROCOCO_API int32 HashArg(cstr s, int64 length)
	{
		struct ANON
		{
			static int jenkins_one_at_a_time_hash(cstr s, int64 len)
			{
				int32 hash = 0;
				for (int64 i = 0; i < len; ++i)
				{
					hash += s[i];
					hash += (hash << 10);
					hash ^= (hash >> 6);
				}
				hash += (hash << 3);
				hash ^= (hash >> 11);
				hash += (hash << 15);
				return hash;
			}
		};

		if (s == nullptr) return -1LL;
		return ANON::jenkins_one_at_a_time_hash(s, length);
	}

	ROCOCO_API void Populate(cr_substring item, IStringPopulator& populator)
	{
		if (!item) return;

		auto len = Length(item);
		char* stackbuffer = (char*)alloca(len + 1);
		memcpy_s(stackbuffer, len + 1, item.start, len);
		stackbuffer[len] = 0;

		populator.Populate(stackbuffer);
	}

	ROCOCO_API cstr ReverseFind(char c, cr_substring token)
	{
		if (!token) return nullptr;

		for (cstr p = token.finish - 1; p >= token.start; p--)
		{
			if (*p == c)
			{
				return p;
			}
		}

		return nullptr;
	}

	ROCOCO_API cstr FindChar(cstr token, char c)
	{
		for (cstr p = token; *p != 0; p++)
		{
			if (*p == c)
			{
				return p;
			}
		}

		return nullptr;
	}

	ROCOCO_API cstr SkipBlankspace(cr_substring token)
	{
		for (cstr p = token.start; p != token.finish; p++)
		{
			if (*p > 32)
			{
				return p;
			}
		}

		return token.finish;
	}

	ROCOCO_API cstr SkipNotBlankspace(cr_substring token)
	{
		for (cstr p = token.start; p != token.finish; p++)
		{
			if (*p <= 32)
			{
				return p;
			}
		}

		return token.finish;
	}

	ROCOCO_API Substring RightOfFirstChar(char c, cr_substring token)
	{
		if (token)
		{
			for (cstr p = token.start; p < token.finish; ++p)
			{
				if (*p == c)
				{
					return Substring{ p + 1, token.finish };
				}
			}
		}

		return Substring::Null();
	}

	ROCOCO_API void Substring::CopyWithTruncate(char* buffer, size_t capacity) const
	{
		if (!buffer || capacity == 0)
		{
			return;
		}

		if (empty())
		{
			*buffer = 0;
			return;
		}

		size_t len = Length();
		size_t writeLength = min(len, capacity - 1);
		memcpy(buffer, start, writeLength);
		buffer[writeLength] = 0;
	}

	ROCOCO_API Substring Substring::ToSubstring(cstr text)
	{
		return text ? Substring{ text, strlen(text) + text } : Substring::Null();
	}

	// N.B sexy script language string length is int32 with max 2^31-1 chars
	ROCOCO_API int32 StringLength(const char* s)
	{
		enum { MAX_INT32 = 0x7FFFFFFF };
		size_t l = strlen(s);
		if (l > MAX_INT32)
		{
			Throw(0, "The string length exceeded INT_MAX characters");
		}

		return (int32)l;
	}

#ifdef ROCOCO_WIDECHAR_IS_WCHAR_T

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

#endif

	ROCOCO_API int WriteToStandardOutput(const char* format, ...)
	{
		va_list args;
		va_start(args, format);

#ifdef _WIN32
		return vprintf_s(format, args);
#else
		return vprintf(format, args);
#endif
	}

#ifdef _WIN32
	ROCOCO_API void CopyString(char* dest, size_t capacity, const char* source)
	{
		strcpy_s(dest, capacity, source);
	}

	ROCOCO_API void CopyString(char* dest, size_t capacity, const char* source, size_t nChars)
	{
		strncpy_s(dest, capacity, source, nChars);
	}
#else
	ROCOCO_API void CopyString(char* dest, size_t capacity, const char* source)
	{
		strncpy(dest, source, capacity);
	}
#endif

#ifdef _WIN32
	ROCOCO_API void StringCat(char* buf, const char* source, int maxChars)
	{
		strcat_s(buf, maxChars, source);
	}

	ROCOCO_API void StringCat(wchar_t* buf, crwstr source, int maxChars)
	{
		wcscat_s(buf, maxChars, source);
	}
#else
	ROCOCO_API void StringCat(char* buf, const char* source, int maxChars)
	{
		strncat(buf, source, maxChars);
	}
#endif

	ROCOCO_API void Assign(U8FilePath& dest, const char* src)
	{
		Format(dest, "%s", src);
	}

	ROCOCO_API void Assign(U8FilePath& dest, crwstr wideSrc)
	{
		Format(dest, "%ls", wideSrc);
	}

	ROCOCO_API void Assign(WideFilePath& dest, const char* src)
	{
		Format(dest, _RW_TEXT("%hs"), src);
	}

	ROCOCO_API void Assign(U32FilePath& dest, const char32_t* wideSrc)
	{
		if (wideSrc == nullptr) wideSrc = U"<null>";

		auto* end = dest.buf + U32FilePath::CAPACITY;

		char32_t* target = dest.buf;
		for (const char32_t* src = wideSrc; *src != 0; ++src, ++target)
		{
			if (target == end)
			{
				goto error;
			}

			*target = *src;
		}

		if (target == end)
		{
			goto error;
		}

		*target = 0;
		return;
	error:
		Throw(0, "Failed to assign UTF-32 string - truncation. UTF-32 is used internally for path directories. Since truncation of  paths is a security problem the function must throw an exception");
	}

	ROCOCO_API size_t rlen(cstr s)
	{
		return strlen(s);
	}

	ROCOCO_API int SafeFormat(char* buffer, size_t capacity, const char* format, ...)
	{
		va_list args;
		va_start(args, format);
		return SafeVFormat(buffer, capacity, format, args);
	}

#ifdef ROCOCO_WIDECHAR_IS_WCHAR_T
	ROCOCO_API int SafeVFormat(wchar_t* buffer, size_t capacity, crwstr format, va_list args)
	{

		int count = _vsnwprintf_s(buffer, capacity, capacity, format, args);

		if (count >= capacity)
		{
			return -1;
		}

		return count;
	}
#endif

	ROCOCO_API int SafeFormat(ROCOCO_WIDECHAR* buffer, size_t capacity, crwstr format, ...)
	{
		va_list args;
		va_start(args, format);
		return SafeVFormat(buffer, capacity, format, args);
	}

	ROCOCO_API int SecureFormat(char* buffer, size_t capacity, const char* format, ...)
	{
		va_list args;
		va_start(args, format);
		int count = SafeVFormat(buffer, capacity, format, args);
		if (count == -1)
		{
			Throw(0, "SecureFormat failed. Buffer length exceeded. Format String: %s", format);
		}
		return count;
	}

	ROCOCO_API int SecureFormat(ROCOCO_WIDECHAR* buffer, size_t capacity, crwstr format, ...)
	{
		va_list args;
		va_start(args, format);
		int count = SafeVFormat(buffer, capacity, format, args);
		if (count == -1)
		{
			Throw(0, "SecureFormat failed. Buffer length exceeded. Format String: %ls", format);
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

	ROCOCO_API int StrCmpN(cstr a, cstr b, size_t len)
	{
		return strncmp(a, b, len);
	}

	ROCOCO_API void SplitString(const char* text, size_t length, IStringPopulator& onSubString, cstr delimiter)
	{
		if (length == 0) length = rlen(text);
		size_t bytecount = sizeof(char) * (length + 1);
		char* buf = (char*)alloca(bytecount);
		memcpy_s(buf, bytecount, text, bytecount);
		buf[length] = 0;

		char* next_token = nullptr;
		char* token = strtok_s(buf, delimiter, &next_token);
		while (token != nullptr)
		{
			onSubString.Populate(token);
			token = strtok_s(nullptr, delimiter, &next_token);
		}
	}

	ROCOCO_API size_t CountSubStrings(cstr text, size_t length, cstr delimiter)
	{
		struct : IStringPopulator
		{
			size_t count;
			void Populate(cstr text) override
			{
				UNUSED(text);
				count++;
			}
		} cb;

		cb.count = 0;
		SplitString(text, length, cb, delimiter);
		return cb.count;
	}

	ROCOCO_API uint32 FastHash(cstr text)
	{
		if (text == nullptr) return 0;

		uint32 hash = 5381;

		while (true)
		{
			int c = *text;
			if (c == 0) break;
			hash = ((hash << 5) + hash) + c;
			text++;
		}

		return hash;
	}

	ROCOCO_API cstr GetFinalNull(cstr s)
	{
		cstr p = s;
		while (*p++ != 0);
		return p - 1;
	}

	ROCOCO_API crwstr GetFinalNull(crwstr s)
	{
		crwstr p = s;
		while (*p++ != 0);
		return p - 1;
	}

	ROCOCO_API cstr GetRightSubstringAfter(cstr s, char c)
	{
		cstr p = GetFinalNull(s);
		for (cstr q = p; q >= s; --q)
		{
			if (*q == c)
			{
				return q;
			}
		}

		return nullptr;
	}

	ROCOCO_API crwstr GetRightSubstringAfter(crwstr s, wchar_t c)
	{
		crwstr p = GetFinalNull(s);
		for (crwstr q = p; q >= s; --q)
		{
			if (*q == c)
			{
				return q;
			}
		}

		return nullptr;
	}

	ROCOCO_API cstr GetFileExtension(cstr s)
	{
		return GetRightSubstringAfter(s, '.');
	}

	ROCOCO_API crwstr GetFileExtension(crwstr s)
	{
		return GetRightSubstringAfter(s, L'.');
	}

	ROCOCO_API bool Eq(const char* a, const char* b)
	{
		return strcmp(a, b) == 0;
	}

#ifdef ROCOCO_WIDECHAR_IS_WCHAR_T
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

	ROCOCO_API bool EndsWith(crwstr bigString, crwstr suffix)
	{
		size_t len = wcslen(suffix);
		size_t lenBig = wcslen(bigString);
		crwstr t = bigString + lenBig - len;
		return Eq(suffix, t);
	}
#endif

#ifdef _WIN32
	ROCOCO_API bool EqI(const char* a, const char* b)
	{
		return _strcmpi(a, b) == 0;
	}
#else
	ROCOCO_API bool EqI(const char* a, const char* b)
	{
		return strcasecmp(a, b) == 0;
	}
#endif

	ROCOCO_API bool StartsWith(cstr bigString, cstr prefix)
	{
		return strncmp(bigString, prefix, strlen(prefix)) == 0;
	}

	ROCOCO_API bool EndsWith(cstr bigString, cstr suffix)
	{
		size_t len = strlen(suffix);
		size_t lenBig = strlen(bigString);
		const char* t = bigString + lenBig - len;
		return Eq(suffix, t);
	}

	ROCOCO_API bool EndsWithI(cstr bigString, cstr suffix)
	{
		size_t len = strlen(suffix);
		size_t lenBig = strlen(bigString);
		const char* t = bigString + lenBig - len;

#ifdef _WIN32
		return _strcmpi(suffix, t) == 0;
#else
		return strcasecmp(suffix, t) == 0;
#endif
	}

	ROCOCO_API StackStringBuilder::StackStringBuilder(char* _buffer, size_t _capacity) :
		buffer(_buffer), capacity(_capacity), length(0)
	{
		buffer[0] = 0;
	}

	ROCOCO_API StackStringBuilder::StackStringBuilder(char* _buffer, size_t _capacity, CursorState type) :
		buffer(_buffer), capacity(_capacity)
	{
		UNUSED(type);
		size_t ulen = strlen(buffer);
		if (ulen > 0x000000007FFFFFFF)
		{
			Throw(0, "Maximum string length exceeded");
		}
		length = (int32)ulen;
		if (length >= capacity)
		{
			length = (int32)(capacity - 1);
			buffer[capacity - 1] = 0;
		}
	}

	ROCOCO_API int32 StackStringBuilder::Length() const
	{
		return length;
	}

	ROCOCO_API StringBuilder& StackStringBuilder::AppendFormat(const char* format, ...)
	{
		size_t ulen = (size_t)length;
		if (ulen == capacity) return *this;

		va_list args;
		va_start(args, format);
		int charsCopied = SafeVFormat(buffer + ulen, capacity - ulen, format, args);
		if (charsCopied > 0)
		{
			length += charsCopied;
			if ((size_t)length > capacity)
			{
				if (Rococo::OS::IsDebugging())
				{
					// vsnprintf_s appears buggy
					Rococo::OS::TripDebugger();
				}
				Throw(0, "_vsnprintf_s appears to be bugged");
			}
		}
		else if (charsCopied < 0)
		{
			length = (int32)(capacity - 1);
			buffer[length] = 0;
		}

		return *this;
	}

	ROCOCO_API StringBuilder& StackStringBuilder::operator << (cstr text)
	{
		return AppendFormat("%s", text);
	}

	ROCOCO_API StringBuilder& StackStringBuilder::AppendChar(char c)
	{
		return AppendFormat("%c", c);
	}

	ROCOCO_API StringBuilder& StackStringBuilder::operator << (int32 value)
	{
		return AppendFormat("%d", value);
	}

	ROCOCO_API StringBuilder& StackStringBuilder::operator << (uint32 value)
	{
		return AppendFormat("%u", value);
	}

	ROCOCO_API StringBuilder& StackStringBuilder::operator << (int64 value)
	{
		return AppendFormat("%lld", value);
	}

	ROCOCO_API StringBuilder& StackStringBuilder::operator << (uint64 value)
	{
		return AppendFormat("%llu", value);
	}

	ROCOCO_API StringBuilder& StackStringBuilder::operator << (float value)
	{
		return AppendFormat("%f", value);
	}

	ROCOCO_API StringBuilder& StackStringBuilder::operator << (double value)
	{
		return AppendFormat("%lf", value);
	}

	ROCOCO_API void StackStringBuilder::Clear()
	{
		buffer[0] = 0;
		length = 0;
	}

	ROCOCO_API bool IsFQNamespace(cr_substring s)
	{
		if (s.empty())
		{
			return false;
		}

		if (s.start == nullptr)
		{
			return false;
		}

		if (s.Length() > MAX_FQ_NAME_LEN)
		{
			return false;
		}

		enum State
		{
			State_ExpectingSubspace,
			State_InSupspace,
		} state = State_ExpectingSubspace;

		for (auto* p = s.start; p != s.finish; p++)
		{
			if (state == State_ExpectingSubspace)
			{
				if ((*p >= 'A' && *p <= 'Z'))
				{
					// Dandy
					state = State_InSupspace;
				}
				else
				{
					return false;
				}
			}
			else // Insubspace
			{
				if ((*p >= 'a' && *p <= 'z') || (*p >= 'A' && *p <= 'Z') || (*p >= '0' && *p <= '9'))
				{
					// Dandy
				}
				else if (*p == '.')
				{
					state = State_ExpectingSubspace;
				}
				else
				{
					return false;
				}
			}
		}

		if (state == State_ExpectingSubspace)
		{
			return false;
		}

		return true;
	}

	ROCOCO_API void ValidateFQNamespace(cstr fqName)
	{
		if (fqName == nullptr)
		{
			Throw(0, "Error validating fully qualified namespace - nul");
		}

		if (*fqName == 0)
		{
			Throw(0, "Error validating fully qualified namespace - blank");
		}

		if (rlen(fqName) > MAX_FQ_NAME_LEN)
		{
			Throw(0, "Error validating fully qualified namespace - exceeded maximum of %d chars", MAX_FQ_NAME_LEN);
		}

		enum State
		{
			State_ExpectingSubspace,
			State_InSupspace,
		} state = State_ExpectingSubspace;

		for (auto* p = fqName; *p != 0; p++)
		{
			if (state == State_ExpectingSubspace)
			{
				if ((*p >= 'A' && *p <= 'Z'))
				{
					// Dandy
					state = State_InSupspace;
				}
				else
				{
					size_t pos = p - fqName;
					Throw(0, "Error validating fully qualified name at pos %llu - Capital letters A-Z only", pos);
				}
			}
			else // Insubspace
			{
				if ((*p >= 'a' && *p <= 'z') || (*p >= 'A' && *p <= 'Z') || (*p >= '0' && *p <= '9'))
				{
					// Dandy
				}
				else if (*p == '.')
				{
					state = State_ExpectingSubspace;
				}
				else
				{
					size_t pos = p - fqName;
					Throw(0, "Error validating fully qualified name - Characters must be in range 0-9 or a-z or A-Z at pos %llu. Use '.' to separate subspaces", pos);
				}
			}
		}

		if (state == State_ExpectingSubspace)
		{
			Throw(0, "Error validating fully qualified name - name must not terminate on a period '.'");
		}
	}

	ROCOCO_API void ValidateFQNameIdentifier(cstr fqName)
	{
		if (fqName == nullptr)
		{
			Throw(0, "Error validating fully qualified name - nul");
		}

		if (*fqName == 0)
		{
			Throw(0, "Error validating fully qualified name - blank");
		}

		if (rlen(fqName) > MAX_FQ_NAME_LEN)
		{
			Throw(0, "Error validating fully qualified name - exceeded maximum of %d chars", MAX_FQ_NAME_LEN);
		}

		enum State
		{
			State_ExpectingSubspace,
			State_InSupspace,
		} state = State_ExpectingSubspace;

		for (auto* p = fqName; *p != 0; p++)
		{
			if (state == State_ExpectingSubspace)
			{
				if ((*p >= 'a' && *p <= 'z') || (*p >= '0' && *p <= '9'))
				{
					// Dandy
					state = State_InSupspace;
				}
				else
				{
					Throw(0, "Error validating fully qualified name - Characters must be in range 0-9 or a-z");
				}
			}
			else // Insubspace
			{
				if ((*p >= 'a' && *p <= 'z') || (*p >= '0' && *p <= '9'))
				{
					// Dandy
				}
				else if (*p == '.')
				{
					state = State_ExpectingSubspace;
				}
				else
				{
					Throw(0, "Error validating fully qualified name - Characters must be in range 0-9 or a-z. Use '.' to separate subspaces");
				}
			}
		}

		if (state == State_ExpectingSubspace)
		{
			Throw(0, "Error validating fully qualified name - name must not terminate on a period '.'");
		}
	}

#ifndef _WIN32
# define _stricmp strcasecmp
# define _strnicmp strncasecmp
#endif

	ROCOCO_API int32 Compare(cstr a, cstr b) { return strcmp(a, b); }
	ROCOCO_API int32 CompareI(cstr a, cstr b) { return _stricmp(a, b); }
	ROCOCO_API int32 CompareI(cstr a, cstr b, size_t count) { return _strnicmp(a, b, count); }
	ROCOCO_API int32 Compare(cstr a, cstr b, size_t count) { return strncmp(a, b, count); }

#ifdef _WIN32
	ROCOCO_API int32 Compare(crwstr a, crwstr b) { return wcscmp(a, b); }
	ROCOCO_API int32 CompareI(crwstr a, crwstr b) { return _wcsicmp(a, b); }
	ROCOCO_API int32 CompareI(crwstr a, crwstr b, size_t count) { return _wcsnicmp(a, b, count); }
	ROCOCO_API int32 Compare(crwstr a, crwstr b, size_t count) { return wcsncmp(a, b, count); }
#endif

	ROCOCO_API const char* GetSubString(const char* s, const char* subString) { return strstr(s, subString); }

	ROCOCO_API IDynamicStringBuilder* CreateDynamicStringBuilder(size_t initialCapacity)
	{
		return new StringsAnon::DynamicStringBuilder(initialCapacity);
	}
} // Rococo

#ifdef _WIN32
#include "xxhash.hpp"

namespace Rococo::Strings
{
	ROCOCO_API uint64 XXHash64Arg(const void* buffer, size_t nBytesLength)
	{
		xxh::hash_t<64> hash = xxh::xxhash<64>(buffer, nBytesLength);
		return hash;
	}
}
#endif

namespace Rococo::Strings
{
	// This is very slow algorithm that requires deep stack recursion for even modest sized strings
	ROCOCO_API int LevenshteinDistance(cstr source, cstr target)
	{
		int lenSrc = StringLength(source);
		int lenTarget = StringLength(target);

		if (lenSrc == 0) { return lenTarget; }
		if (lenTarget == 0) { return lenSrc; }

		int distance = toupper(source[lenSrc - 1]) == toupper(target[lenTarget - 1]) ? 0 : 1;

		char* sourcePrefix = (char*)alloca(lenSrc);
		char* targetPrefix = (char*)alloca(lenTarget);

		memcpy(sourcePrefix, source, lenSrc - 1);
		memcpy(targetPrefix, target, lenTarget - 1);
		sourcePrefix[lenSrc - 1] = 0;
		targetPrefix[lenTarget - 1] = 0;

		return min(min(LevenshteinDistance(sourcePrefix, target) + 1,
			LevenshteinDistance(source, targetPrefix)) + 1,
			LevenshteinDistance(sourcePrefix, targetPrefix) + distance);
	}

	ROCOCO_API void ReplaceChar(char* buffer, size_t capacity, char target, char replacement)
	{
		for (char* c = buffer; c < buffer + capacity; c++)
		{
			if (*c == 0) return;

			if (*c == target)
			{
				*c = replacement;
			}
		}
	}

	ROCOCO_API bool StartsWith(cr_substring token, const fstring& prefix)
	{
		size_t len = token.finish - token.start;
		return prefix.length <= len && strncmp(prefix, token.start, prefix.length) == 0;
	}

	ROCOCO_API bool StartsWith(cstr token, cr_substring prefix)
	{
		size_t len = Length(prefix);
		return strncmp(token, prefix.start, len) == 0;
	}

	ROCOCO_API ptrdiff_t Length(cr_substring token)
	{
		return token.finish - token.start;
	}

	ROCOCO_API bool Substring::TryCopyWithoutTruncate(char* outputBuffer, size_t sizeofOutputBuffer) const
	{
		if (Length() >= (ptrdiff_t)sizeofOutputBuffer)
		{
			return false;
		}

		char* writePtr = outputBuffer;
		cstr readPtr = start;
		while (readPtr < finish)
		{
			*writePtr++ = *readPtr++;
		}

		*writePtr = 0;

		return true;
	}

	ROCOCO_API bool Eq(cr_substring b, cstr a)
	{
		cstr p = a;
		cstr q = b.start;
		for (; q != b.finish; ++q, ++p)
		{
			if (*p != *q) return false;
			if (*p == 0) return false;
		}

		return *p == 0;
	}

	ROCOCO_API bool Eq(const fstring& a, cr_substring b)
	{
		if (a.length != Length(b))
		{
			return false;
		}

		cstr p = a.buffer;
		cstr q = b.start;
		for (; q != b.finish; ++q, ++p)
		{
			if (*p != *q) return false;
		}

		return true;
	}

	ROCOCO_API bool Eq(cr_substring a, const fstring& b)
	{
		return Eq(b, a);
	}

	ROCOCO_API bool IsEmpty(cr_substring token)
	{
		return token.start == token.finish;
	}

	ROCOCO_API bool Eq(cr_substring a, cr_substring b)
	{
		auto lenA = Length(a);
		auto lenB = Length(b);

		if (lenA != lenB)
		{
			return false;
		}

		cstr p = a.start;
		cstr q = b.start;
		for (; p < a.finish; p++, q++)
		{
			if (*p != *q)
			{
				return false;
			}
		}

		return true;
	}
}

namespace Rococo::Reflection
{
	ROCOCO_API void ReflectStackFormat(Reflection::IReflectionVisitor& v, cstr name, const char* format, ...)
	{
		char text[1024];
		va_list args;
		va_start(args, format);
		SafeVFormat(text, sizeof(text), format, args);
		va_end(args);

		auto readOnly = Reflection::ReflectionMetaData::ReadOnly();
		v.Reflect(name, text, readOnly);
	}

	void EnterSection(Reflection::IReflectionVisitor& v, const char* format, ...)
	{
		char text[256];
		va_list args;
		va_start(args, format);
		SafeVFormat(text, sizeof(text), format, args);
		va_end(args);

		v.EnterSection(text);
	}

	void EnterElement(Reflection::IReflectionVisitor& v, const char* format, ...)
	{
		char text[256];
		va_list args;
		va_start(args, format);
		SafeVFormat(text, sizeof(text), format, args);
		va_end(args);

		v.EnterElement(text);
	}
}

namespace Rococo::Sex::Inference
{
	ROCOCO_API bool IsNotTokenChar(char c)
	{
		return !IsAlphaNumeric(c) && c != '.' && c != '#';
	}

	ROCOCO_API bool IsSexyKeyword(cr_substring candidate)
	{
		size_t len = Length(candidate);

		static std::vector<fstring> keywords
		{
			"method"_fstring, "function"_fstring, "class"_fstring, "struct"_fstring
		};

		for (auto keyword : keywords)
		{
			if (StartsWith(candidate, keyword))
			{
				if (len > keyword.length && IsNotTokenChar(candidate.start[keyword.length]))
				{
					// We found a keyword, but we do not need to parse it
					return true;
				}
			}
		}

		return false;
	}

	ROCOCO_API cstr GetFirstNonTokenPointer(cr_substring s)
	{
		if (!s) return nullptr;

		for (cstr p = s.start; p < s.finish; ++p)
		{
			if (!IsNotTokenChar(*p))
			{
				return p;
			}
		}

		return s.finish;
	}

	ROCOCO_API cstr GetFirstNonTokenPointerFromRight(cr_substring doc, cstr startPosition)
	{
		if (!startPosition || !doc) return nullptr;

		for (cstr p = startPosition - 1; p >= doc.start; p--)
		{
			if (IsNotTokenChar(*p))
			{
				return p;
			}
		}

		return nullptr;
	}

	ROCOCO_API cstr GetFirstNonTypeCharPointer(cr_substring s)
	{
		bool inDot = false;

		for (cstr p = s.start; p < s.finish; ++p)
		{
			if (!inDot)
			{
				if (*p == '.')
				{
					inDot = true;
					continue;
				}
			}

			if (IsAlphaNumeric(*p) || *p == '#')
			{
				if (inDot)
				{
					inDot = false;
				}

				continue;
			}

			return p;
		}

		return s.finish;
	}

	ROCOCO_API Substring GetFirstTokenFromLeft(cr_substring s)
	{
		return s ? Substring{ s.start, GetFirstNonTypeCharPointer(s) } : Substring::Null();
	}
} // Rococo::Strings

namespace Rococo
{
	ROCOCO_API fstring to_fstring(cstr const msg)
	{
		size_t len = rlen(msg);
		if (len >= (uint64) Limits::FSTRING_LENGTH_LIMIT)
		{
			Throw(0, "String too long to convert to fstring");
		}
		return{ msg, (int)len };
	}

	ROCOCO_API IExpandingBuffer* CreateExpandingBuffer(size_t initialCapacity)
	{
		return new StringsAnon::ExpandingBuffer(initialCapacity);
	}

	ROCOCO_API bool operator == (const fstring& a, const fstring& b)
	{
		return a.length == b.length && Strings::StrCmpN(a.buffer, b.buffer, a.length) == 0;
	}
} // Rococo

namespace Rococo::Strings::CLI
{
	ROCOCO_API void GetCommandLineArgument(const fstring& prefix, cstr commandLine, char* buffer, size_t capacity, cstr defaultString)
	{
		cstr directive = strstr(commandLine, prefix);
		if (directive == nullptr)
		{
			CopyString(buffer, capacity, defaultString);
			return;
		}

		bool isQuoted = directive[prefix.length] == '\"';

		Substring arg;
		if (isQuoted)
		{
			arg.start = directive + prefix.length + 1;
			cstr p = arg.start;
			for (;;)
			{
				if (*p == 0 || *p == '\"')
				{
					arg.finish = p;
					break;
				}

				p++;
			}
		}
		else
		{
			arg.start = directive + prefix.length;
			cstr p;
			for (p = arg.start; !isblank(*p) && *p != 0; p++)
			{
			}
			arg.finish = p + 1;
		}

		if (arg)
		{
			arg.CopyWithTruncate(buffer, capacity);
		}
		else
		{
			CopyString(buffer, capacity, defaultString);
		}
	}
}