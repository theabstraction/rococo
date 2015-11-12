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
			buffer = new wchar_t[capacity];
			buffer[0] = 0;
		}

		virtual operator const wchar_t* () const
		{
			return buffer;
		}

		virtual int AppendFormat(const wchar_t* format, ...)
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

		wchar_t* buffer;
		size_t capacity;
		size_t offset;
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
	fstring to_fstring(const wchar_t* const msg)
	{
		size_t len = wcslen(msg);
		if (len >= 0x020000000LL)
		{
			Throw(0, L"String too long to convert to fstring");
		}
		return{ msg, (int)len };
	}

	bool operator == (const fstring& a, const fstring& b)
	{
		return a.length == b.length && wcsncmp(a.buffer, b.buffer, a.length) == 0;
	}

	IStringBuilder* CreateSafeStringBuilder(size_t capacity)
	{
		return new SafeStringBuilder(capacity);
	}

	void SplitString(const wchar_t* text, size_t length, const wchar_t* seperators, IEventCallback<const wchar_t*>& onSubString)
	{
		if (length == 0) length = wcslen(text);
		size_t bytecount = sizeof(wchar_t) * (length + 1);
		wchar_t* buf = (wchar_t*)_alloca(bytecount);
		memcpy_s(buf, bytecount, text, bytecount);
		buf[length] = 0;

		wchar_t* next_token = nullptr;
		wchar_t* token = wcstok_s(buf, L"|", &next_token);
		while (token != nullptr)
		{
			onSubString.OnEvent(token);
			token = wcstok_s(nullptr, L"|", &next_token);
		}
	}

	size_t CountSubStrings(const wchar_t* text, size_t length, const wchar_t* seperators)
	{
		struct : IEventCallback<const wchar_t*>
		{
			size_t count;
			virtual void OnEvent(const wchar_t* text)
			{
				count++;
			}
		} cb;

		cb.count = 0;
		SplitString(text, length, seperators, cb);
		return cb.count;
	}

	uint32 FastHash(const wchar_t* text)
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
}