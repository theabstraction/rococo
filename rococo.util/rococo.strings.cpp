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

		virtual const uint8* GetData() const
		{
			if (internalBuffer.empty()) return nullptr;
			else return &internalBuffer[0];
		}

		virtual uint8* GetData()
		{
			if (internalBuffer.empty()) return nullptr;
			else return &internalBuffer[0];
		}

		virtual size_t Length() const
		{
			return internalBuffer.size();
		}

		virtual void Resize(size_t length)
		{
			internalBuffer.resize(length);
		}

		virtual void Free()
		{
			delete this;
		}
	};
}

namespace Rococo
{
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
		size_t bytecount = sizeof(rchar) * (length + 1);
		rchar* buf = (rchar*)alloca(bytecount);
		memcpy_s(buf, bytecount, text, bytecount);
		buf[length] = 0;

		rchar* next_token = nullptr;
		rchar* token = strtok_s(buf, "|", &next_token);
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

   cstr GetRightSubstringAfter(cstr s, rchar c)
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

   cstr GetFileExtension(cstr s)
   {
      return GetRightSubstringAfter(s, L'.');
   }

   bool Eq(const char* a, const char* b)
   {
      return strcmp(a, b) == 0;
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
}