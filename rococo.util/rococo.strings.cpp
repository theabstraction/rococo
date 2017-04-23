#include <rococo.api.h>
#include <stdarg.h>
#include <malloc.h>
#include <vector>
#include <rococo.io.h>
#include <rococo.strings.h>

namespace
{
	using namespace Rococo;

	class SafeStringBuilder : public IStringBuilder
	{
	public:
		SafeStringBuilder(size_t _capacity): buffer(nullptr), capacity(_capacity), offset(0)
		{
			buffer = new rchar[capacity];
			buffer[0] = 0;
		}

		virtual operator cstr () const
		{
			return buffer;
		}

		virtual int AppendFormat(cstr format, ...)
		{
			if (offset >= capacity - 1)
			{
				return 0;
			}

			va_list args;
			va_start(args, format);
			int result = SafeVFormat(buffer + offset, capacity - offset, _TRUNCATE, format, args);

			if (result > 0)
			{
				offset += result;
			}
			return result;
		}

		virtual void Free()
		{
			delete this;
		}
	private:
		~SafeStringBuilder()
		{
			delete[] buffer;
		}

		rchar* buffer;
		size_t capacity;
		size_t offset;
	};

	class SafeStackStringBuilder : public IStringBuilder
	{
	private:
		SafeStackString& sss;
		size_t offset;
	public:
		SafeStackStringBuilder(SafeStackString& _sss) : sss(_sss), offset(0)
		{
			
		}

		virtual operator cstr () const
		{
			return sss.Buffer();
		}

		virtual int AppendFormat(cstr format, ...)
		{
			if (offset >= sss.Capacity() - 1)
			{
				return 0;
			}

			va_list args;
			va_start(args, format);
			int result = SafeVFormat(sss.Buffer() + offset, sss.Capacity() - offset, _TRUNCATE, format, args);

			if (result > 0)
			{
				offset += result;
			}
			return result;
		}

		virtual void Free()
		{
		}
	};

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

	IStringBuilder* CreateSafeStringBuilder(size_t capacity)
	{
		return new SafeStringBuilder(capacity);
	}

	IStringBuilder* CreateSafeStackStringBuilder(SafeStackString& sss)
	{
		static_assert(sizeof(SafeStackStringBuilder) < SafeStackString::OPAQUE_CAPACITY, "OPAQUE_CAPACITY insufficient");
		return new (sss.Data()) SafeStackStringBuilder(sss);
	}

	void SplitString(const char* text, size_t length, cstr seperators, IEventCallback<cstr>& onSubString)
	{
		if (length == 0) length = rlen(text);
		size_t bytecount = sizeof(rchar) * (length + 1);
		rchar* buf = (rchar*)_alloca(bytecount);
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
		int c;

		while(c = *text++)
		{
			hash = ((hash << 5) + hash) + c;
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
}