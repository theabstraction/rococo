/*
Sexy Scripting Language - Copright(c)2013. Mark Anthony Taylor. All rights reserved.

http://www.sexiestengine.com

Email: mark.anthony.taylor@gmail.com

The Sexy copyright holder is Mark Anthony Taylor, the English author of 'Lazy Bloke Ghosts of Parliament', 'Lazy Bloke in the 35th Century', 'King of the Republic', 'Origin of the Species'
and 'Society of Demons.'

1. This software is open-source. It can be freely copied, distributed, compiled and the compilation executed.

1.a Modification of the software is not allowed where such modifcations fail to include this exact comment header in each source file, or alter, reduce or extend the Sexy language.
The purpose of this limitation is to prevent different incompatible versions of the Sexy language emerging.

1.b You are allowed to fix bugs and implement performance improvements providing you inform Sexy's copyright owner via email. Such changes may then be incorporated in
later versions of Sexy without compromising Sexy's copyright license.

2. You are not permitted to copyright derivative versions of the source code. You are free to compile the code into binary libraries and include the binaries in a commercial application.

3. THERE IS NO WARRANTY FOR THE PROGRAM, TO THE EXTENT PERMITTED BY APPLICABLE LAW. EXCEPT WHEN OTHERWISE STATED IN WRITING THE COPYRIGHT HOLDERS AND/OR OTHER PARTIES PROVIDE THE PROGRAM �AS IS� WITHOUT
WARRANTY OF ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE. THE ENTIRE RISK AS TO THE QUALITY
AND PERFORMANCE OF THE PROGRAM IS WITH YOU. SHOULD THE PROGRAM PROVE DEFECTIVE, YOU ASSUME THE COST OF ALL NECESSARY SERVICING, REPAIR OR CORRECTION.

4. IN NO EVENT UNLESS REQUIRED BY APPLICABLE LAW OR AGREED TO IN WRITING WILL ANY COPYRIGHT HOLDER, OR ANY OTHER PARTY WHO MODIFIES AND/OR CONVEYS THE PROGRAM AS PERMITTED ABOVE, BE LIABLE TO YOU FOR
DAMAGES, INCLUDING ANY GENERAL, SPECIAL, INCIDENTAL OR CONSEQUENTIAL DAMAGES ARISING OUT OF THE USE OR INABILITY TO USE THE PROGRAM (INCLUDING BUT NOT LIMITED TO LOSS OF DATA OR DATA BEING RENDERED
INACCURATE OR LOSSES SUSTAINED BY YOU OR THIRD PARTIES OR A FAILURE OF THE PROGRAM TO OPERATE WITH ANY OTHER PROGRAMS), EVEN IF SUCH HOLDER OR OTHER PARTY HAS BEEN ADVISED OF THE POSSIBILITY OF
SUCH DAMAGES.

5. Any program that distributes this software in source or binary form must include an obvious credit to the the language, its copyright holder, and the language homepage. This must be the case in its
principal credit screen and its principal readme file.
*/

#include <sexy.types.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <stdbool.h>
#include <sys/types.h>
#include <unistd.h>
#include <sys/sysctl.h>

#define BREAK_ON_THROW 1

namespace
{
   int breakFlags = 0;
}

namespace Sexy
{
   void memcpy_s(void *dest, size_t destSize, const void *src, size_t count)
   {
      memcpy(dest, src, count);
   }

   int sscanf_s(const char* buffer, const char* format, ...)
   {
      va_list args;
      va_start(args, format);
      return vsscanf(buffer, format, args);
   }

   int sprintf_s(char* buffer, size_t capacity, const char* format, ...)
   {
      va_list args;
      va_start(args, format);
      return vsnprintf(buffer, capacity, format, args);
   }

   int _vsnprintf_s(char* buffer, size_t capacity, size_t maxCount, const char* format, va_list args)
   {
      return vsnprintf(buffer, capacity, format, args);
   }

   void strcpy_s(char* dest, size_t capacity, const char* source)
   {
      strcpy(dest, source);
   }

   void strncpy_s(char* dest, size_t capacity, const char* source, size_t maxCount)
   {
      strncpy(dest, source, maxCount);
   }

   void strcat_s(char* dest, size_t capacity, const char* source)
   {
      strcat(dest, source);
   }

   void strncat_s(char* dest, size_t capacity, const char* source, size_t maxCount)
   {
      strncpy(dest, source, maxCount);
   }

   bool IsDebuggerPresent()
   {
      int                 junk;
      int                 mib[4];
      struct kinfo_proc   info;
      size_t              size;

      // Initialize the flags so that, if sysctl fails for some bizarre 
      // reason, we get a predictable result.

      info.kp_proc.p_flag = 0;

      // Initialize mib, which tells sysctl the info we want, in this case
      // we're looking for information about a specific process ID.

      mib[0] = CTL_KERN;
      mib[1] = KERN_PROC;
      mib[2] = KERN_PROC_PID;
      mib[3] = getpid();

      // Call sysctl.

      size = sizeof(info);
      junk = sysctl(mib, sizeof(mib) / sizeof(*mib), &info, &size, NULL, 0);

      // We're being debugged if the P_TRACED flag is set.

      return ((info.kp_proc.p_flag & P_TRACED) != 0);
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
		   if ((breakFlags & flag) != 0 && IsDebuggerPresent())
		   {
			   __builtin_trap();
		   }
	   }
   #else
	   void BreakOnThrow(BreakFlag flag) {}
   #endif
   }
}