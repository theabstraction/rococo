#include <rococo.types.h>
#include <stdarg.h>
#include <rococo.strings.h>

namespace Rococo
{
   namespace OS
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

         SafeVFormat(ex.msg, _TRUNCATE, format, args);

         ex.errorCode = errorCode;

         OS::TripDebugger();

         throw ex;
      }
   }
}
