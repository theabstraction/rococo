#include <rococo.debugging.h>

#define ROCOCO_USE_SAFE_V_FORMAT
#include <rococo.strings.h>

#include <stdio.h>

#include <list>
#include <vector>

#include <rococo.api.h>

#define BREAK_ON_THROW 1

namespace
{
   int breakFlags = 0;
}

namespace Rococo
{
   bool IsPointerValid(const void* ptr)
   {
      return ptr != nullptr;
   }

   namespace OS
   {
      void SetBreakPoints(int flags)
      {
         static_assert(sizeof(int64) == 8, "Bad int64");
         static_assert(sizeof(int32) == 4, "Bad int32");
         static_assert(sizeof(int16) == 2, "Bad int16");
         static_assert(sizeof(int8) == 1, "Bad int8");
         breakFlags = flags;
      }

#ifdef BREAK_ON_THROW
      void BreakOnThrow(BreakFlag flag)
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
	void Throw(int32 errorCode, cstr format, ...)
	{
		using namespace Rococo::Debugging;

		va_list args;
		va_start(args, format);

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
		} ex;

		SafeVFormat(ex.msg, sizeof(ex.msg), format, args);

		ex.errorCode = errorCode;

		OS::BreakOnThrow((OS::BreakFlag)0x7FFFFFFF);

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

	namespace OS
	{
		void LoadAsciiTextFile(IEventCallback<cstr>& onLoad, const wchar_t* filename)
		{
			std::vector<char> asciiData;

			struct AutoFile
			{
				FILE* fp = nullptr;

				~AutoFile()
				{
					if (fp != nullptr)
					{
						fclose(fp);
					}
				}
			};

			{ // autofile section
#ifdef _WIN32
# pragma warning(disable: 4996)
#endif
				AutoFile f{ _wfopen(filename, L"rb") };

				_fseeki64(f.fp, 0, SEEK_END);
				long long nCapacity = _ftelli64(f.fp);
				_fseeki64(f.fp, 0, SEEK_SET);

				asciiData.reserve(nCapacity + 1);

#ifdef _WIN32
# pragma warning(default: 4996)
#endif
				if (f.fp == nullptr)
				{
					Throw(0, "Cannot open file %S", filename);
				}

				while (true)
				{
					char data[8192];
					size_t bytesRead = fread(data, 1, sizeof(data), f.fp);
					if (bytesRead <= 0)
					{
						asciiData.push_back(0);
						break;
					}

					for (auto i = 0; i < bytesRead; ++i)
					{
						asciiData.push_back(data[i]);
					}
				}
			}

			onLoad.OnEvent(asciiData.data());
		}

		void LoadAsciiTextFile(char* data, size_t capacity, const wchar_t* filename)
		{
#ifdef _WIN32
# pragma warning(disable: 4996)
#endif

			FILE* f = _wfopen(filename, L"rb");

#ifdef _WIN32
# pragma warning(default: 4996)
#endif
			if (f == nullptr)
			{
				Throw(0, "Cannot open file %S", filename);
			}

			size_t startIndex = 0;
			while (startIndex < capacity)
			{
				size_t bytesRead = fread(data + startIndex, 1, capacity - startIndex, f);

				if (bytesRead == 0)
				{
					// graceful completion
					break;
				}
				startIndex += bytesRead;
			}

			if (startIndex >= capacity)
			{
				fclose(f);
				Throw(0, "File too large: %S", filename);
			}

			fclose(f);

			data[startIndex] = 0;
		}
	} // OS
} // Rococo