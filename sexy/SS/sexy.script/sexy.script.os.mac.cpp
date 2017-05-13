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

	3. THERE IS NO WARRANTY FOR THE PROGRAM, TO THE EXTENT PERMITTED BY APPLICABLE LAW. EXCEPT WHEN OTHERWISE STATED IN WRITING THE COPYRIGHT HOLDERS AND/OR OTHER PARTIES PROVIDE THE PROGRAM “AS IS” WITHOUT
	WARRANTY OF ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE. THE ENTIRE RISK AS TO THE QUALITY
	AND PERFORMANCE OF THE PROGRAM IS WITH YOU. SHOULD THE PROGRAM PROVE DEFECTIVE, YOU ASSUME THE COST OF ALL NECESSARY SERVICING, REPAIR OR CORRECTION.

	4. IN NO EVENT UNLESS REQUIRED BY APPLICABLE LAW OR AGREED TO IN WRITING WILL ANY COPYRIGHT HOLDER, OR ANY OTHER PARTY WHO MODIFIES AND/OR CONVEYS THE PROGRAM AS PERMITTED ABOVE, BE LIABLE TO YOU FOR
	DAMAGES, INCLUDING ANY GENERAL, SPECIAL, INCIDENTAL OR CONSEQUENTIAL DAMAGES ARISING OUT OF THE USE OR INABILITY TO USE THE PROGRAM (INCLUDING BUT NOT LIMITED TO LOSS OF DATA OR DATA BEING RENDERED
	INACCURATE OR LOSSES SUSTAINED BY YOU OR THIRD PARTIES OR A FAILURE OF THE PROGRAM TO OPERATE WITH ANY OTHER PROGRAMS), EVEN IF SUCH HOLDER OR OTHER PARTY HAS BEEN ADVISED OF THE POSSIBILITY OF
	SUCH DAMAGES.

	5. Any program that distributes this software in source or binary form must include an obvious credit to the the language, its copyright holder, and the language homepage. This must be the case in its
	principal credit screen and its principal readme file.
*/

#include "sexy.script.stdafx.h"

#include "sexy.debug.types.h"
#include "sexy.compiler.public.h"
#include "sexy.script.h"

#include <cerrno>
#include <memory>
#include <stdarg.h>
#include <stdio.h>
#include <dlfcn.h>
#include <unistd.h>
#include <mach-o/dyld.h>

#define BREAK_ON_THROW

namespace Rococo { namespace OS 
{
   void GetDefaultNativeSrcDir(SEXCHAR* data, size_t capacity)
   {
      uint32 lcapacity = (uint32) capacity;
      if (_NSGetExecutablePath(data, &lcapacity) != 0)
      {
         Throw(0, SEXTEXT("_NSGetExecutablePath failed"));
      }

      while (StripLastSubpath(data))
      {
         SEXCHAR fullpath[_MAX_PATH];
         StringPrint(fullpath, _MAX_PATH, SEXTEXT("%s%s"), data, SEXTEXT("src_indicator.txt"));
         if (OS::IsFileExistant(fullpath))
         {
            StringCat(data, SEXTEXT("NativeSource/"), (int32)capacity);
            return;
         }
      }

      Rococo::Throw(0, SEXTEXT("SEXY_NATIVE_SRC_DIR. Failed to get default variable: cannot find src_indicator.txt descending from sexy.script.dll"));
   }

#include <sys/stat.h>

   void GetEnvVariable(SEXCHAR* data, size_t capacity, const SEXCHAR* envVariable)
   {
      if (AreEqual(envVariable, SEXTEXT("SEXY_NATIVE_SRC_DIR")))
      {
         GetDefaultNativeSrcDir(data, capacity);

         struct stat sb;
         if (stat(data, &sb) != 0 || !S_ISDIR(sb.st_mode))
         {
            Rococo::Throw(0, SEXTEXT("Error associating environment variable %s to the sexy native source directory"), envVariable);
         }

         return;
      }

      Throw(0, SEXTEXT("Environment variable %s not found"), envVariable);
   }

	void LoadAsciiTextFile(SEXCHAR* data, size_t capacity, const SEXCHAR* filename)
	{
		FILE* f = fopen(filename, "rb");
		if (f == nullptr)
		{
			Throw(0, SEXTEXT("Cannot open file %s"), filename);
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

	typedef void (*FN_AddNativeSexyCalls)(Rococo::Script::IScriptSystem& ss);

   Rococo::Script::FN_CreateLib GetLibCreateFunction(const SEXCHAR* dynamicLinkLibOfNativeCalls, bool throwOnError)
	{
	   SEXCHAR linkLib[_MAX_PATH];
		StringPrint(linkLib, _MAX_PATH, SEXTEXT("%s.mac.dylib"), dynamicLinkLibOfNativeCalls);
      void* lib = dlopen(linkLib, RTLD_NOW | RTLD_GLOBAL);
      if (lib == nullptr)
		{
         if (throwOnError) Rococo::Throw(errno, SEXTEXT("Could not load %s"), (const SEXCHAR*) linkLib);
         return nullptr;
		}

      Rococo::Script::FN_CreateLib fp;
      *(void **)(&fp) = dlsym(lib, "CreateLib");
		if (fp == nullptr)
		{
         if (throwOnError) Rococo::Throw(errno, SEXTEXT("Could not find INativeLib* CreateLib(...) in %s"), linkLib);
         return nullptr;
		}

		return fp;
	}
}} // Rococo::OS