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

#ifdef char_IS_WIDE
# define _UNICODE
# define UNICODE
#endif

#include <rococo.win32.target.win7.h>
#define WIN32_LEAN_AND_MEAN
#include <Windows.h>

#include "sexy.debug.types.h"
#include "sexy.compiler.public.h"
#include "sexy.script.h"

#include <memory>

#include <stdarg.h>

#define BREAK_ON_THROW

#ifndef SCRIPT_IS_LIBRARY
BOOL WINAPI DllMain(HINSTANCE hinstDLL, DWORD fdwReason, LPVOID lpvReserved)
{
	return TRUE;
}
#endif

namespace
{
	void CloseAndThrowOS(HANDLE toClose, const char* message, const char* filename)
	{
      CloseHandle(toClose);
		Rococo::Throw(GetLastError(), ("%s: %s"), message, filename);
	}
}

namespace Rococo {
	namespace OS
	{
		void GetDefaultNativeSrcDir(char* data, size_t capacity)
		{
#ifdef SCRIPT_IS_LIBRARY
			GetCurrentDirectory((DWORD)capacity, data);
#else
			HMODULE hModule = GetModuleHandle(("sexy.script.dll"));
			if (hModule == NULL)
			{
				Rococo::OS::OSException ose;
				ose.exceptionNumber = GetLastError();
				Rococo::StringPrint(ose.message, 256, ("SEXY_NATIVE_SRC_DIR. Failed to get default variable: cannot get module handle for sexy.script.dll"));
				throw ose;
			}

			GetModuleFileName(hModule, data, (DWORD)capacity);
#endif

			while (StripLastSubpath(data))
			{
				char fullpath[_MAX_PATH];
				SafeFormat(fullpath, _MAX_PATH, ("%s%s"), data, ("src_indicator.txt"));
				if (IsFileExistant(fullpath))
				{
					StringCat(data, ("NativeSource\\"), (int32)capacity);
					return;
				}
			}

			Rococo::Throw(GetLastError(), ("SEXY_NATIVE_SRC_DIR. Failed to get default variable: cannot find src_indicator.txt descending from sexy.script.dll"));
		}

		void GetEnvVariable(char* data, size_t capacity, const char* envVariable)
		{
			if (0 == GetEnvironmentVariable(envVariable, data, (DWORD)capacity))
			{
				if (AreEqual(envVariable, ("SEXY_NATIVE_SRC_DIR")))
				{
					GetDefaultNativeSrcDir(data, capacity);

					if (INVALID_FILE_ATTRIBUTES == GetFileAttributes(data))
					{
						Rococo::Throw(GetLastError(), ("Error associating environment variable %s to the sexy native source directory"), envVariable);
					}

					SetEnvironmentVariable(envVariable, data);
					return;
				}

				Throw(GetLastError(), ("Environment variable %s not found"), envVariable);
			}
		}

		typedef void(*FN_AddNativeSexyCalls)(Rococo::Script::IScriptSystem& ss);

		Rococo::Script::FN_CreateLib GetLibCreateFunction(const char* dynamicLinkLibOfNativeCalls, bool throwOnError)
		{
			char linkLib[_MAX_PATH];
			SafeFormat(linkLib, _MAX_PATH, ("%s.dll"), dynamicLinkLibOfNativeCalls);
			HMODULE lib = LoadLibrary(linkLib);
			if (lib == nullptr)
			{
				if (throwOnError) Rococo::Throw(GetLastError(), ("Could not load %s"), (const char*)linkLib);
				return nullptr;
			}

			FARPROC fp = GetProcAddress(lib, "CreateLib");
			if (fp == nullptr)
			{
				if (throwOnError) Rococo::Throw(GetLastError(), ("Could not find  INativeLib* CreateLib(...) in %s"), linkLib);
				return nullptr;
			}

			Rococo::Script::FN_CreateLib createFn = (Rococo::Script::FN_CreateLib) fp;
			return createFn;
		}
	}
} // Rococo::OS