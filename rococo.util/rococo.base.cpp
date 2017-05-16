#include <stdarg.h>

#include <rococo.types.h>
#include <rococo.strings.h>

#include <stdio.h>

namespace Rococo
{
   void Throw(int32 errorCode, cstr format, ...)
   {
      va_list args;
      va_start(args, format);

      struct : public IException
      {
         rchar msg[256];
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

      _vsnprintf_s(ex.msg, 256, _TRUNCATE, format, args);

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