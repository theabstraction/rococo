#include <stdarg.h>

#include <rococo.types.h>

#define ROCOCO_USE_SAFE_V_FORMAT
#include <rococo.strings.h>

#include <stdio.h>

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
      va_list args;
      va_start(args, format);

      struct : public IException
      {
         rchar msg[2048];
         int32 errorCode;

         virtual cstr Message() const
         {
            return msg;
         }

         virtual int32 ErrorCode() const
         {
            return errorCode;
         }
      } ex;

      SafeVFormat(ex.msg, sizeof(ex.msg), format, args);

      ex.errorCode = errorCode;

      OS::BreakOnThrow((OS::BreakFlag)0x7FFFFFFF);

      throw ex;
   }

   void LoadAsciiTextFile(char* data, size_t capacity, const char* filename)
   {
#ifdef _WIN32
# pragma warning(disable: 4996)
#endif

      FILE* f = fopen(filename, "rb");

#ifdef _WIN32
# pragma warning(default: 4996)
#endif
      if (f == nullptr)
      {
         Throw(0, "Cannot open file %s", filename);
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
         Throw(0, "File too large: ", filename);
      }

      fclose(f);

      data[startIndex] = 0;
   }
}