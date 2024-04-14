#define ROCOCO_API __declspec(dllexport)
#define ROCOCO_ID_API ROCOCO_API
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

#include <rococo.ids.h>
#include <rococo.os.h>
#include <rococo.time.h>
#include <atomic>
#include <ctype.h>
#include <immintrin.h>

namespace Rococo
{
	ROCOCO_API bool IsPointerValid(const void* ptr)
	{
		return ptr != nullptr;
	}

	namespace Ids
	{
		static std::atomic<int32> uniqueCounter = 0;

		ROCOCO_ID_API UniqueIdHolder MakeNewUniqueId()
		{
			UniqueIdHolder id;

			struct Username : IStringPopulator
			{
				uint64 hash = 0;

				void Populate(cstr text) override
				{
					char cipherCrud[256];
					SafeFormat(cipherCrud, "%s_%lld_%lld", text, (int64)Rococo::OS::GetCurrentThreadIdentifier(), (int64) Rococo::Time::UTCTime());
					hash = Strings::XXHash64Arg(cipherCrud, strlen(cipherCrud));
					memset(cipherCrud, 0, sizeof cipherCrud);
				}
			} username;

			Rococo::OS::GetCurrentUserName(username);

			uint64 randValue;
			if (0 == _rdrand64_step(&randValue))
			{
				randValue = Rococo::Time::TickCount();
			}

			randValue &= 0xFFFF'FFFF'0000'0000ULL;

			int32 next = uniqueCounter++;
			int64 next64 = randValue | (int64)next;

			id.iValues[0] = next64;
			id.iValues[1] = username.hash ^ Rococo::Time::UTCTime();

			return id;
		}

		struct MehGuid
		{
			// Example: 30dd879c-ee2f-11db-8314-0800200c9a66
			uint32 a;
			uint16 b;
			uint16 c;
			uint16 d;
			uint16 e;
			uint32 f;
		};

		union GuidAndUniqueId
		{
			GuidAndUniqueId() : id(), guid() {}
			UniqueIdHolder id;
			MehGuid guid;
		};

		static_assert(sizeof MehGuid == sizeof UniqueIdHolder);
		static_assert(sizeof GuidAndUniqueId == 16);

		ROCOCO_ID_API void PopulateAsGuid(UniqueIdHolder id, IStringPopulator& populator)
		{
			GuidAndUniqueId glue;
			glue.id = id;

			MehGuid g = glue.guid;

			// Example: 30dd879c-ee2f-11db-8314-0800200c9a66
			char guidBuffer[40];
			SafeFormat(guidBuffer, "%8.8x-%4.4x-%4.4x-%4.4x-%4.4x%8.8x", g.a, g.b, g.c, g.d, g.e, g.f);
			populator.Populate(guidBuffer);
		}

		bool TryGrabHexToken(OUT uint32& value, cstr start, cstr end)
		{
			char buffer[16];

			char* dest = buffer;

			for (cstr s = start; s < end; s++)
			{
				char c = *s;
				if (!isalnum(c))
				{
					return false;
				}

				*dest++ = c;
			}

			*dest++ = 0;

			if (sscanf_s(buffer, "%x", &value) != 1)
			{
				return false;
			}

			return true;
		}

		ROCOCO_ID_API bool TryScanGuid(OUT UniqueIdHolder& id, cstr buffer)
		{
			if (strlen(buffer) != 36)
			{
				return false;
			}

			uint32 a; // the first 8 digits
			if (!TryGrabHexToken(a, buffer, buffer + 8))
			{
				return false;
			}

			uint32 b; // the next 4 digits
			if (!TryGrabHexToken(b, buffer + 9, buffer + 13))
			{
				return false;
			}

			uint32 c; // the next 4 digits
			if (!TryGrabHexToken(c, buffer + 14, buffer + 18))
			{
				return false;
			}

			uint32 d; // the next 4 digits
			if (!TryGrabHexToken(d, buffer + 19, buffer + 23))
			{
				return false;
			}

			uint32 e; // the first 4 digits of the final 12 digit block
			if (!TryGrabHexToken(e, buffer + 24, buffer + 28))
			{
				return false;
			}

			uint32 f; // The final 8 digits of the final 12 digit block
			if (!TryGrabHexToken(f, buffer + 28, buffer + 36))
			{
				return false;
			}

			GuidAndUniqueId glue;
			glue.guid.a = a;
			glue.guid.b = (uint16)b;
			glue.guid.c = (uint16)c;
			glue.guid.d = (uint16)d;
			glue.guid.e = (uint16)e;
			glue.guid.f = f;

			id = glue.id;

			return true;
		}
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

	ROCOCO_API void ThrowMissingResourceFile(ErrorCode /* code */, cstr description, cstr filename)
	{
		struct MissingResourceFile : IException
		{
			char message[1024] = { 0 };
			Rococo::ErrorCode code;

			cstr Message() const override
			{
				return message;
			}

			int ErrorCode() const override
			{
				return (int) code;
			}

			Debugging::IStackFrameEnumerator* StackFrames() override
			{
				return nullptr;
			}
		} err;

		SafeFormat(err.message, "%s: %s", description, filename);
		throw err;
	}

	ROCOCO_API void ThrowIllFormedSExpression(int32 displacement, cstr format, ...)
	{
		va_list args;
		va_start(args, format);

		RococoException ex;

		SafeVFormat(ex.msg, sizeof(ex.msg), format, args);

		ex.errorCode = displacement;

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

	ROCOCO_API_EXPORT void ForEachCriticalLog(Strings::IStringPopulator& onMessage)
	{
		for (auto& i : rollingLog)
		{
			onMessage.Populate(i.c_str());
		}
	}

	ROCOCO_API_EXPORT void ValidateCriticalLog()
	{
		if (!rollingLog.empty())
		{
			Throw(0, "The critical log has something to add:\n");
		}
	}
}

