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
#include <sexy.script.exports.h>
#include "sexy.native.sys.type.h"

#include "sexy.compiler.h"

#ifdef char_IS_WIDE
# define UNICODE
# define _UNICODE
#endif

#include "rococo.os.win32.h"

using namespace Rococo;
using namespace Rococo::Script;
using namespace Rococo::Compiler;
using namespace Rococo::SysType;
using namespace Rococo::Sex;

#include "sys.reflection.inl"

#ifdef _WIN32

using namespace MSWindows;

BOOL DllMain(HINSTANCE hDllHandle, DWORD nReason, LPVOID reserved)
{
	UNUSED(reserved);

    BOOL bSuccess = TRUE;
      
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
   DLLEXPORT INativeLib* CreateLib(Rococo::Script::IScriptSystem& ss)
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
			void AddNativeCalls() override
			{
				AddReflectionCalls(ss);
			}

			void ClearResources() override
			{

			}

			void Release() override
			{
				delete this;
			}
		};

		return new ReflectionNativeLib(ss);
	}
}