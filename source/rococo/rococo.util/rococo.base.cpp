#define ROCOCO_API __declspec(dllexport)
#include <rococo.debugging.h>

#define ROCOCO_USE_SAFE_V_FORMAT
#include <rococo.strings.h>

#include <stdio.h>
#include <list>
#include <vector>

#define BREAK_ON_THROW 1

using namespace Rococo::Strings;

namespace
{
   int breakFlags = 0;
}

#ifdef __APPLE__
namespace Rococo
{
	FILE* _wfopen(const wchar_t* filename, const wchar_t* mode);
}
#endif

namespace Rococo
{
	ROCOCO_API bool IsPointerValid(const void* ptr)
   {
      return ptr != nullptr;
   }

   namespace IO
   {
	   ROCOCO_API void ToU8(const U32FilePath& src, U8FilePath& dest)
	   {
		   char* q = dest.buf;
		   const char32_t* p = src;

		   while (*p != 0)
		   {
			   char32_t c = *p;

			   if (c < 32 || c > 127)
			   {
				   Throw(0, "Cannot convert Unicode to ascii. Character value out of range at position #llu", p - src.buf);
			   }
			   else
			   {
				   *q = (char)*p;
			   }

			   p++, q++;
		   }

		   *q = 0;
	   }

	   ROCOCO_API void ToWide(const U32FilePath& src, WideFilePath& dest)
	   {
		   wchar_t* q = dest.buf;
		   const char32_t* p = src;

		   while (*p != 0)
		   {
			   char32_t c = *p;

			   if (c < 32 || c > 32767)
			   {
				   Throw(0, "Cannot convert Unicode to wide char. Character value out of range at position #llu", p - src.buf);
			   }
			   else
			   {
				   *q = (char)*p;
			   }

			   p++, q++;
		   }

		   *q = 0;
	   }

	   ROCOCO_API void PathFromAscii(cstr ascii_string, U32FilePath& path)
	   {
		   char32_t* q = path.buf;
		   const char* p = ascii_string;

		   while (*p != 0)
		   {
			   if (*p < 32 || *p > 127)
			   {
				   Throw(0, "Cannot convert 8-bit to char32_t. Character value out of range at position #llu", p - ascii_string);
			   }
			   *q++ = *p++;
		   }

		   *q = 0;
	   }

	   ROCOCO_API void PathFromWide(const wchar_t* wide_string, U32FilePath& path)
	   {
		   char32_t* q = path.buf;
		   const wchar_t* p = wide_string;

		   while (*p != 0)
		   {
			   if (*p < 32 || *p >= 32767)
			   {
				   Throw(0, "Cannot convert wide to char32_t. Character value out of range at position #llu", p - wide_string);
			   }
			   *q++ = *p++;
		   }

		   *q = 0;
	   }
   }

   namespace OS
   {
	   ROCOCO_API void SetBreakPoints(int flags)
      {
         static_assert(sizeof(int64) == 8, "Bad int64");
         static_assert(sizeof(int32) == 4, "Bad int32");
         static_assert(sizeof(int16) == 2, "Bad int16");
         static_assert(sizeof(int8) == 1, "Bad int8");
         breakFlags = flags;
      }

#ifdef BREAK_ON_THROW
	   ROCOCO_API void BreakOnThrow(Flags::BreakFlag flag)
      {
         if ((breakFlags & flag) != 0 && Rococo::OS::IsDebugging())
         {
            TripDebugger();
         }
      }
#else
      void BreakOnThrow(BreakFlag flag) {}
#endif
   }
}

namespace Rococo
{
	using namespace Rococo::Debugging;

	struct RococoException : public IException, public IStackFrameEnumerator
	{
		char msg[2048];
		int32 errorCode;

		std::list<StackFrame> stackFrames;

		cstr Message() const override
		{
			return msg;
		}

		int32 ErrorCode() const override
		{
			return errorCode;
		}

		IStackFrameEnumerator* StackFrames() override
		{
			return this;
		}

		void FormatEachStackFrame(IStackFrameFormatter& formatter) override
		{
			for (auto& i : stackFrames)
			{
				formatter.Format(i);
			}
		}
	};

	ROCOCO_API void Throw(int32 errorCode, cstr format, ...)
	{
		va_list args;
		va_start(args, format);

		RococoException ex;

		SafeVFormat(ex.msg, sizeof(ex.msg), format, args);

		ex.errorCode = errorCode;

		OS::BreakOnThrow((OS::Flags::BreakFlag)0x7FFFFFFF);

		struct ANON : public IStackFrameFormatter
		{
			RococoException* ex;
			void Format(const StackFrame& sf)
			{
				ex->stackFrames.push_back(sf);
			}
		} formatter;
		formatter.ex = &ex;

		FormatStackFrames(formatter);

		throw ex;
	}

	ROCOCO_API void ThrowIllFormedSExpression(int32 errorCode, cstr format, ...)
	{
		va_list args;
		va_start(args, format);

		RococoException ex;

		SafeVFormat(ex.msg, sizeof(ex.msg), format, args);

		ex.errorCode = errorCode;

		OS::BreakOnThrow(OS::Flags::BreakFlag_IllFormed_SExpression);

		struct ANON : public IStackFrameFormatter
		{
			RococoException* ex;
			void Format(const StackFrame& sf)
			{
				ex->stackFrames.push_back(sf);
			}
		} formatter;
		formatter.ex = &ex;

		FormatStackFrames(formatter);

		throw ex;
	}

	namespace Maths::IEEE475
	{
		ROCOCO_API float BinaryToFloat(uint32 binaryRepresentation)
		{
			return *(float*)(&binaryRepresentation);
		}

		ROCOCO_API double BinaryToDouble(uint64 binaryRepresentation)
		{
			return *(double*)(&binaryRepresentation);
		}

		ROCOCO_API uint32 FloatToBinary(float f)
		{
			return *(uint32*)(&f);
		}

		ROCOCO_API uint64 DoubleToBinary(double d)
		{
			return *(uint64*)(&d);
		}
	}
} // Rococo

#include <string>
#include <list>
#include <allocators/rococo.allocator.template.h>

namespace Rococo::Debugging
{
	typedef std::basic_string<char, std::char_traits<char>, AllocatorWithMalloc<char>> mstring;

	std::list<mstring, AllocatorWithMalloc<mstring>> rollingLog;

	ROCOCO_API_EXPORT void AddCriticalLog(cstr message)
	{
		rollingLog.push_back(message);
		if (rollingLog.size() > 10)
		{
			rollingLog.pop_front();
		}
	}

	ROCOCO_API_EXPORT void ForEachCriticalLog(IEventCallback<cstr>& onMessage)
	{
		for (auto& i : rollingLog)
		{
			onMessage.OnEvent(i.c_str());
		}
	}
}