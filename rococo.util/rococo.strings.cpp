#include <rococo.api.h>
#include <stdarg.h>

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

#ifdef __APPLE__
# define _stricmp strcasecmp
#endif

namespace
{
	using namespace Rococo;

	class ExpandingBuffer : public IExpandingBuffer
	{
		std::vector<uint8> internalBuffer;
	public:
		ExpandingBuffer(size_t capacity) : internalBuffer(capacity)
		{
		}

		const uint8* GetData() const override
		{
			if (internalBuffer.empty()) return nullptr;
			else return &internalBuffer[0];
		}

		uint8* GetData() override
		{
			if (internalBuffer.empty()) return nullptr;
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

namespace Rococo
{
	int32 Format(U8FilePath& path, cstr format, ...)
	{
		va_list args;
		va_start(args, format);
		int count = SafeVFormat(path.buf, path.CAPACITY, format, args);
		if (count == -1)
		{
			Throw(0, "%s failed. Buffer length exceeded. Format String: %s", __FUNCTION__, format);
		}
		return count;
	}

	int32 Format(WideFilePath& path, const wchar_t* format, ...)
	{
		va_list args;
		va_start(args, format);
		int count = SafeVFormat(path.buf, path.CAPACITY, format, args);
		if (count == -1)
		{
			Throw(0, "%s failed. Buffer length exceeded. Format String: %ls", __FUNCTION__, format);
		}
		return count;
	}

	bool IsCapital(char c)
	{
		return c >= 'A' && c <= 'Z';
	}

	bool IsLowerCase(char c)
	{
		return c >= 'a' && c <= 'z';
	}

	bool IsAlphabetical(char c)
	{
		return IsCapital(c) || IsLowerCase(c);
	}

	bool IsNumeric(char c)
	{
		return c >= '0' && c <= '9';
	}

	bool IsAlphaNumeric(char c)
	{
		return IsAlphabetical(c) || IsNumeric(c);
	}

	int32 Hash(int32 x)
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

	int32 Hash(int64 x)
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

	size_t Hash(cstr s)
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

		if (s == nullptr) return -1;
		return ANON::jenkins_one_at_a_time_hash(s, StringLength(s));
	}

	int32 Hash(cstr s, int64 length)
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

	// N.B sexy script language string length is int32 with max 2^31-1 chars
	int32 StringLength(const char* s)
	{
		enum { MAX_INT32 = 0x7FFFFFFF };
		size_t l = strlen(s);
		if (l > MAX_INT32)
		{
			Throw(0, "The string length exceeded INT_MAX characters");
		}

		return (int32)l;
	}

	// N.B sexy script language string length is int32 with max 2^31-1 chars
	int32 StringLength(const wchar_t* s)
	{
		enum { MAX_INT32 = 0x7FFFFFFF };
		size_t l = wcslen(s);
		if (l > MAX_INT32)
		{
			Throw(0, "The string length exceeded INT_MAX characters");
		}

		return (int32)l;
	}
	int WriteToStandardOutput(const char* format, ...)
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
	void CopyString(char* dest, size_t capacity, const char* source)
	{
		strcpy_s(dest, capacity, source);
	}
#else
	void CopyString(char* dest, size_t capacity, const char* source)
	{
		strncpy(dest, source, capacity);
	}
#endif

#ifdef _WIN32
	void StringCat(char* buf, const char* source, int maxChars)
	{
		strcat_s(buf, maxChars, source);
	}

	void StringCat(wchar_t* buf, const wchar_t* source, int maxChars)
	{
		wcscat_s(buf, maxChars, source);
	}
#else
	void StringCat(char* buf, const char* source, int maxChars)
	{
		strncat(buf, source, maxChars);
	}
#endif

	void Assign(U8FilePath& dest, const wchar_t* wideSrc)
	{
		Format(dest, "%ls", wideSrc);
	}

	void Assign(WideFilePath& dest, const char* src)
	{
		Format(dest, L"%hs", src);
	}

   size_t rlen(cstr s)
   {
      return strlen(s);
   }

   int SafeFormat(char* buffer, size_t capacity, const char* format, ...)
   {
      va_list args;
      va_start(args, format);
      return SafeVFormat(buffer, capacity, format, args);
   }

   int SafeVFormat(wchar_t* buffer, size_t capacity, const wchar_t* format, va_list args)
   {
#ifdef _WIN32
	   int count = _vsnwprintf_s(buffer, capacity, capacity, format, args);
#else
	   auto count = vswprintf(buffer, capacity, format, args); 
#endif
		   
	   if (count >= capacity)
	   {
		   return -1;
	   }

	   return count;
   }

   int SafeFormat(wchar_t* buffer, size_t capacity, const wchar_t* format, ...)
   {
	   va_list args;
	   va_start(args, format);
	   return SafeVFormat(buffer, capacity, format, args);
   }

   int SecureFormat(char* buffer, size_t capacity, const char* format, ...)
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

   int SecureFormat(wchar_t* buffer, size_t capacity, const wchar_t* format, ...)
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

   int SafeVFormat(char* buffer, size_t capacity, const char* format, va_list args)
   {
      int count = vsnprintf(buffer, capacity, format, args);
      if (count >= capacity)
      {
         return -1;
      }

      return count;
   }

	fstring to_fstring(cstr const msg)
	{
		size_t len = rlen(msg);
		if (len >= 0x020000000LL)
		{
			Throw(0, "String too long to convert to fstring");
		}
		return{ msg, (int)len };
	}

   int StrCmpN(cstr a, cstr b, size_t len)
   {
      return strncmp(a, b, len);
   }

	bool operator == (const fstring& a, const fstring& b)
	{
		return a.length == b.length && StrCmpN(a.buffer, b.buffer, a.length) == 0;
	}

	void SplitString(const char* text, size_t length, cstr seperators, IEventCallback<cstr>& onSubString)
	{
		if (length == 0) length = rlen(text);
		size_t bytecount = sizeof(char) * (length + 1);
		char* buf = (char*)alloca(bytecount);
		memcpy_s(buf, bytecount, text, bytecount);
		buf[length] = 0;

		char* next_token = nullptr;
		char* token = strtok_s(buf, "|", &next_token);
		while (token != nullptr)
		{
			onSubString.OnEvent(token);
			token = strtok_s(nullptr, "|", &next_token);
		}
	}

	size_t CountSubStrings(cstr text, size_t length, cstr seperators)
	{
		struct : IEventCallback<cstr>
		{
			size_t count;
			virtual void OnEvent(cstr text)
			{
				count++;
			}
		} cb;

		cb.count = 0;
		SplitString(text, length, seperators, cb);
		return cb.count;
	}

	uint32 FastHash(cstr text)
	{
		if (text == nullptr) return 0;

		uint32 hash = 5381;

		while(true)
		{
         int c = *text;
         if (c == 0) break;
			hash = ((hash << 5) + hash) + c;
         text++;
		}

		return hash;
	}

	IExpandingBuffer* CreateExpandingBuffer(size_t initialCapacity)
	{
		return new ExpandingBuffer(initialCapacity);
	}

   cstr GetFinalNull(cstr s)
   {
      cstr p = s;
      while (*p++ != 0);
      return p - 1;
   }

   const wchar_t* GetFinalNull(const wchar_t* s)
   {
	   const wchar_t* p = s;
	   while (*p++ != 0);
	   return p - 1;
   }

   cstr GetRightSubstringAfter(cstr s, char c)
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

   const wchar_t* GetRightSubstringAfter(const wchar_t* s, wchar_t c)
   {
	   const wchar_t* p = GetFinalNull(s);
	   for (const wchar_t* q = p; q >= s; --q)
	   {
		   if (*q == c)
		   {
			   return q;
		   }
	   }

	   return nullptr;
   }

   cstr GetFileExtension(cstr s)
   {
      return GetRightSubstringAfter(s, '.');
   }

   const wchar_t* GetFileExtension(const wchar_t* s)
   {
	   return GetRightSubstringAfter(s, L'.');
   }

   bool Eq(const char* a, const char* b)
   {
      return strcmp(a, b) == 0;
   }

   bool Eq(const wchar_t* a, const wchar_t* b)
   {
	   return wcscmp(a, b) == 0;
   }

   bool EqI(const char* a, const char* b)
   {
	   return _stricmp(a, b) == 0;
   }

   bool StartsWith(cstr bigString, cstr prefix)
   {
	   return strncmp(bigString, prefix, strlen(prefix)) == 0;
   }

   bool EndsWith(cstr bigString, cstr suffix)
   {
	   size_t len = strlen(suffix);
	   size_t lenBig = strlen(bigString);
	   const char* t = bigString + lenBig - len;
	   return Eq(suffix, t);
   }

   bool StartsWith(const wchar_t* bigString, const wchar_t* prefix)
   {
	   return wcsncmp(bigString, prefix, wcslen(prefix)) == 0;
   }

   bool EndsWith(const wchar_t* bigString, const wchar_t* suffix)
   {
	   size_t len = wcslen(suffix);
	   size_t lenBig = wcslen(bigString);
	   const wchar_t* t = bigString + lenBig - len;
	   return Eq(suffix, t);
   }

   StackStringBuilder::StackStringBuilder(char* _buffer, size_t _capacity) :
      buffer(_buffer), capacity(_capacity), length(0)
   {
      buffer[0] = 0;
   }

   StackStringBuilder::StackStringBuilder(char* _buffer, size_t _capacity, eOpenType type):
      buffer(_buffer), capacity(_capacity)
   {
      size_t ulen = strlen(buffer);
      if (ulen > 0x000000007FFFFFFF)
      {
         Throw(0, "Maximum string length exceeded");
      }
      length = (int32)ulen;
      if (length >= capacity)
      {
         length = (int32) (capacity - 1);
         buffer[capacity - 1] = 0;
      }
   }

   int32 StackStringBuilder::Length() const
   {
      return length;
   }

   StringBuilder& StackStringBuilder::AppendFormat(const char* format, ...)
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

   StringBuilder& StackStringBuilder::operator << (cstr text)
   {
      return AppendFormat("%s", text);
   }

   StringBuilder& StackStringBuilder::AppendChar(char c)
   {
	   return AppendFormat("%c", c);
   }

   StringBuilder& StackStringBuilder::operator << (int32 value)
   {
      return AppendFormat("%d", value);
   }

   StringBuilder& StackStringBuilder::operator << (uint32 value)
   {
      return AppendFormat("%u", value);
   }

   StringBuilder& StackStringBuilder::operator << (int64 value)
   {
      return AppendFormat("%lld", value);
   }

   StringBuilder& StackStringBuilder::operator << (uint64 value)
   {
      return AppendFormat("%llu", value);
   }

   StringBuilder& StackStringBuilder::operator << (float value)
   {
      return AppendFormat("%f", value);
   }

   StringBuilder& StackStringBuilder::operator << (double value)
   {
      return AppendFormat("%lf", value);
   }

   void StackStringBuilder::Clear()
   {
      buffer[0] = 0;
      length = 0;
   }

   void ValidateFQNameIdentifier(cstr fqName)
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

   int32 Compare(cstr a, cstr b) { return strcmp(a, b); }
   int32 CompareI(cstr a, cstr b) { return _stricmp(a, b); }
   int32 CompareI(cstr a, cstr b, int64 count) { return _strnicmp(a, b, count); }
   int32 Compare(cstr a, cstr b, int64 count) { return strncmp(a, b, count); }

   const char* GetSubString(const char* s, const char *subString) { return strstr(s, subString); }

   IDynamicStringBuilder* CreateDynamicStringBuilder(size_t initialCapacity)
   {
	   return new DynamicStringBuilder(initialCapacity);
   }
} // Rococo

#include "xxhash.hpp"

namespace Rococo
{
	uint64 XXHash64(const void* buffer, size_t nBytesLength)
	{
		xxh::hash_t<64> hash = xxh::xxhash<64>(buffer, nBytesLength);
		return hash;
	}

	// This is very slow algorithm that requires deep stack recursion for even modest sized strings
	int LevenshteinDistance(cstr source, cstr target)
	{
		int lenSrc = StringLength(source);
		int lenTarget = StringLength(target);

		if (lenSrc == 0) { return lenTarget; }
		if (lenTarget == 0) { return lenSrc;  }

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
}