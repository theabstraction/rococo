#include <rococo.types.h>
#include <wchar.h>
#include <stdarg.h>

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
}

namespace Rococo
{
	IStringBuilder* CreateSafeStringBuilder(size_t capacity)
	{
		return new SafeStringBuilder(capacity);
	}
}