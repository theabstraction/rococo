// sexy.nativelib.maths.cpp : Defines the exported functions for the DLL application.
//

#include "sexy.nativelib.maths.stdafx.h"
#include "sexy.types.h"
#include "sexy.debug.types.h"
#include "sexy.compiler.public.h"
#include "sexy.vm.h"
#include "sexy.vm.cpu.h"
#include "sexy.script.h"
#include "sexy.native.sys.type.h"

#include "rococo.os.win32.h"

using namespace Rococo;
using namespace Rococo::Script;
using namespace Rococo::Compiler;
using namespace Rococo::SysType;
using namespace Rococo::Sex;



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
   DLLEXPORT INativeLib* CreateLib(Rococo::Script::IScriptSystem& ss)
	{
		class MathsNativeLib: public INativeLib
		{
		private:
			IScriptSystem& ss;

		public:
			MathsNativeLib(IScriptSystem& _ss): ss(_ss)
			{
			}

		private:
			virtual void AddNativeCalls()
			{
				const INamespace& ns = ss.AddNativeNamespace("Sys");
			}

			virtual void ClearResources()
			{
			}

			virtual void Release() 
			{
				delete this;
			}
		};
		return new MathsNativeLib(ss);
	}
}


