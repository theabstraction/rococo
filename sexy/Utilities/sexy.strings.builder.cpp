#include <sexy.types.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

namespace Sexy
{
   StackStringBuilder::StackStringBuilder(char* _buffer, size_t _capacity) :
      buffer(_buffer), capacity(_capacity), length(0)
   {
      buffer[0] = 0;
   }

   strbuilder& StackStringBuilder::AppendFormat(const char* format, ...)
   {
      size_t ulen = (size_t) length;
      if (ulen == capacity) return *this;

      va_list args;
      va_start(args, format);
      int charsCopied = _vsnprintf_s(buffer + ulen, capacity - ulen, _TRUNCATE, format, args);
      if (charsCopied > 0)
      {
         length += charsCopied;
         if ((size_t)length > capacity)
         {
            if (Sexy::OS::IsDebuggerPresent())
            {
               // vsnprintf_s appears buggy
               Sexy::OS::TripDebugger();
            }
            abort();
         }
      }    
      
      return *this;
   }

   strbuilder& StackStringBuilder::operator << (csexstr text)
   {
      return AppendFormat("%s", text);
   }

   strbuilder& StackStringBuilder::operator << (int32 value)
   {
      return AppendFormat("%d", value);
   }

   strbuilder& StackStringBuilder::operator << (uint32 value)
   {
      return AppendFormat("%u", value);
   }

   strbuilder& StackStringBuilder::operator << (int64 value)
   {
      return AppendFormat("%lld", value);
   }

   strbuilder& StackStringBuilder::operator << (uint64 value)
   {
      return AppendFormat("%llu", value);
   }
}