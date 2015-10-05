// sexy.nativelib.win32.cpp : Defines the exported functions for the DLL application.
//

#include "sexy.nativelib.reflection.stdafx.h"

#include <rococo.win32.target.win7.h>

#define WIN32_LEAN_AND_MEAN             // Exclude rarely-used stuff from Windows headers

#include "sexy.types.h"
#include "sexy.strings.h"
#include "sexy.debug.types.h"
#include "sexy.compiler.public.h"
#include "sexy.script.h"

#include "sexy.native.sys.type.h"

#ifdef SEXCHAR_IS_WIDE
# define UNICODE
# define _UNICODE
#endif

#include <rococo.win32.target.win7.h>
#define WIN32_LEAN_AND_MEAN
#include <Windows.h>

#include "sexy.vm.cpu.h"

using namespace Sexy;
using namespace Sexy::Script;
using namespace Sexy::Compiler;
using namespace Sexy::SysType;
using namespace Sexy::Sex;

HINSTANCE s_hNativeLibInstance = NULL;

#include "sys.reflection.inl"

BOOLEAN WINAPI DllMain(HINSTANCE hDllHandle, DWORD nReason, LPVOID Reserved)
{
    BOOLEAN bSuccess = TRUE;
      
    switch (nReason)
	{
	case DLL_PROCESS_ATTACH: 
		DisableThreadLibraryCalls(hDllHandle);
		s_hNativeLibInstance = hDllHandle;
		break;
	case DLL_PROCESS_DETACH:
		break;
	}
  
	return bSuccess;
}

extern "C"
{
	__declspec(dllexport) INativeLib* CreateLib(Sexy::Script::IScriptSystem& ss)
	{
		class ReflectionNativeLib: public INativeLib
		{
		private:
			IScriptSystem& ss;

		public:
			ReflectionNativeLib(IScriptSystem& _ss): ss(_ss)
			{
			}

		private:
			virtual void AddNativeCalls()
			{
				AddReflectionCalls(ss);
			}

			virtual void ClearResources()
			{
			}

			virtual void Release() 
			{
				delete this;
			}
		};

		return new ReflectionNativeLib(ss);
	}
}