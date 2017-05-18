// dllmain.cpp : Defines the entry point for the DLL application.
#include <windows.h>

#ifdef _DEBUG
#pragma comment(lib, "rococo.maths.debug.lib")
#pragma comment(lib, "rococo.util.debug.lib")
#else
#pragma comment(lib, "rococo.maths.lib")
#pragma comment(lib, "rococo.util.lib")
#endif

BOOL APIENTRY DllMain( HMODULE hModule, DWORD  ul_reason_for_call,LPVOID lpReserved)
{
	switch (ul_reason_for_call)
	{
	case DLL_PROCESS_ATTACH:
      DisableThreadLibraryCalls(hModule);
	case DLL_THREAD_ATTACH:
	case DLL_THREAD_DETACH:
	case DLL_PROCESS_DETACH:
		break;
	}
	return TRUE;
}

