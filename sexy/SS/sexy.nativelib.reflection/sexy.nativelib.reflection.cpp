// sexy.nativelib.win32.cpp : Defines the exported functions for the DLL application.
//

#include "sexy.nativelib.reflection.stdafx.h"

#include "sexy.types.h"
#include "sexy.strings.h"
#include "sexy.debug.types.h"
#include "sexy.compiler.public.h"
#include "sexy.vm.h"
#include "sexy.vm.cpu.h"
#include "sexy.script.h"

#include "sexy.native.sys.type.h"

#ifdef SEXCHAR_IS_WIDE
# define UNICODE
# define _UNICODE
#endif

#include "rococo.os.win32.h"

using namespace Sexy;
using namespace Sexy::Script;
using namespace Sexy::Compiler;
using namespace Sexy::SysType;
using namespace Sexy::Sex;

#include "sys.reflection.inl"

#ifdef _WIN32

BOOLEAN WINAPI DllMain(HINSTANCE hDllHandle, DWORD nReason, LPVOID Reserved)
{
    BOOLEAN bSuccess = TRUE;
      
    switch (nReason)
	{
	case DLL_PROCESS_ATTACH: 
		DisableThreadLibraryCalls(hDllHandle);
		break;
	case DLL_PROCESS_DETACH:
		break;
	}
  
	return bSuccess;
}

# define DLLEXPORT __declspec(dllexport) 
#else
# define DLLEXPORT
#endif

extern "C"
{
   DLLEXPORT INativeLib* CreateLib(Sexy::Script::IScriptSystem& ss)
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