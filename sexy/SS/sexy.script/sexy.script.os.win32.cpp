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

#ifdef SEXCHAR_IS_WIDE
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
	void CloseAndThrowOS(HANDLE toClose, const Sexy::SEXCHAR* message, const Sexy::SEXCHAR* filename)
	{
      CloseHandle(toClose);
		Sexy::Throw(GetLastError(), SEXTEXT("%s: %s"), message, filename);
	}
}

namespace Sexy { namespace OS 
{
	bool IsFileExistant(const SEXCHAR* filename)
	{
		return INVALID_FILE_ATTRIBUTES != GetFileAttributes(filename);
	}

	bool StripLastSubpath(SEXCHAR* fullpath)
	{
		int32 len = StringLength(fullpath);
		for(int i = len-2; i > 0; --i)
		{
			if (fullpath[i] == (SEXCHAR) '\\')
			{
				fullpath[i+1] = 0;
				return true;
			}
		}

		return false;
	}

	void GetDefaultNativeSrcDir(SEXCHAR* data, size_t capacity)
	{
#ifdef SCRIPT_IS_LIBRARY
		GetCurrentDirectory((DWORD) capacity, data);
#else
		HMODULE hModule = GetModuleHandle(SEXTEXT("sexy.script.dll"));
		if (hModule == NULL)
		{
			Sexy::OS::OSException ose;
			ose.exceptionNumber = GetLastError();
			Sexy::StringPrint(ose.message, 256, SEXTEXT("SEXY_NATIVE_SRC_DIR. Failed to get default variable: cannot get module handle for sexy.script.dll"));
			throw ose;
		}

		GetModuleFileName(hModule, data, (DWORD) capacity);
#endif

		while (StripLastSubpath(data))
		{
			SEXCHAR fullpath[_MAX_PATH];
			StringPrint(fullpath, _MAX_PATH, SEXTEXT("%s%s"), data, SEXTEXT("src_indicator.txt"));
			if (IsFileExistant(fullpath))
			{
				StringCat(data, SEXTEXT("NativeSource\\"), (int32) capacity);
				return;
			}
		}

      Sexy::Throw(GetLastError(), SEXTEXT("SEXY_NATIVE_SRC_DIR. Failed to get default variable: cannot find src_indicator.txt descending from sexy.script.dll"));
	}

	void GetEnvVariable(SEXCHAR* data, size_t capacity, const SEXCHAR* envVariable)
	{
		if (0 == GetEnvironmentVariable(envVariable, data, (DWORD) capacity))
		{
			if (AreEqual(envVariable, SEXTEXT("SEXY_NATIVE_SRC_DIR")))
			{
				GetDefaultNativeSrcDir(data, capacity);

				if (INVALID_FILE_ATTRIBUTES == GetFileAttributes(data))
				{
               Sexy::Throw(GetLastError(), SEXTEXT("Error associating environment variable %s to the sexy native source directory"), envVariable);
				}

				SetEnvironmentVariable(envVariable, data);
				return;
			}

         Throw(GetLastError(), SEXTEXT("Environment variable %s not found"), envVariable);
		}
	}

   void Throw(int errCode, csexstr format, ...)
   {
      class OSException : public Rococo::IException
      {
      public:
         enum { CAPACITY = 1024 };
         int exceptionNumber;
         rchar message[CAPACITY];

         virtual int ErrorCode() const { return exceptionNumber; }
         virtual cstr Message() const { return message; }
      } ex;
      va_list args;
      va_start(args, format);
      SafeVFormat(ex.message, _TRUNCATE, format, args);
      ex.exceptionNumber = errCode;
      throw ex;
   }

	void LoadAsciiTextFile(SEXCHAR* data, size_t capacity, const SEXCHAR* filename)
	{
		char* rawAsciiData = (char*) _alloca(capacity);

		HANDLE hFile = CreateFile(filename, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_READONLY | FILE_FLAG_SEQUENTIAL_SCAN, NULL);
		if (hFile == INVALID_HANDLE_VALUE)
		{
			CloseAndThrowOS(hFile, SEXTEXT("Cannot open file"), filename);
		}

		LARGE_INTEGER len;
		if (!GetFileSizeEx(hFile, &len))
		{				
			CloseAndThrowOS(hFile, SEXTEXT("Cannot get file size"), filename);
		}

		if (len.HighPart != 0 || (LONG) len.LowPart >= (LONG) capacity)
		{
			CloseAndThrowOS(hFile, SEXTEXT("Cannot handle file size"), filename);
		}

		DWORD bytesRead = 0;
		if (!ReadFile(hFile, rawAsciiData, (DWORD) capacity, &bytesRead, NULL))
		{
			CloseAndThrowOS(hFile, SEXTEXT("Cannot read file"), filename);
		}
		
		CloseHandle(hFile);

#ifdef SEXCHAR_IS_WIDE
		for(int i = 0; i < (int) bytesRead; ++i)
		{
			data[i] = (SEXCHAR) rawAsciiData[i];
		}
#else
		memcpy(data, rawAsciiData, bytesRead);		
#endif

		data[bytesRead] = 0;
	}

	typedef void (*FN_AddNativeSexyCalls)(Sexy::Script::IScriptSystem& ss);

   Sexy::Script::FN_CreateLib GetLibCreateFunction(const SEXCHAR* dynamicLinkLibOfNativeCalls, bool throwOnError)
	{
	   SEXCHAR linkLib[_MAX_PATH];
		StringPrint(linkLib, _MAX_PATH, SEXTEXT("%s.dll"), dynamicLinkLibOfNativeCalls);
      HMODULE lib = LoadLibrary(linkLib);
      if (lib == nullptr)
		{
         if (throwOnError) Sexy::Throw(GetLastError(), SEXTEXT("Could not load %s"), (const SEXCHAR*) linkLib);
         return nullptr;
		}

		FARPROC fp = GetProcAddress(lib, "CreateLib");
		if (fp == nullptr)
		{
         if (throwOnError) Sexy::Throw(GetLastError(), SEXTEXT("Could not find  INativeLib* CreateLib(...) in %s"), linkLib);
         return nullptr;
		}

		Sexy::Script::FN_CreateLib createFn = (Sexy::Script::FN_CreateLib) fp;
		return createFn;
	}
}} // Sexy::OS