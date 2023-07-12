#include <rococo.mplat.main.inl>

using namespace Rococo;

BOOL WINAPI DllMain(_In_ HINSTANCE hinstDLL, _In_ DWORD fdwReason, _In_ LPVOID lpvReserved)
{
	UNUSED(hinstDLL);
	UNUSED(fdwReason);
	UNUSED(lpvReserved);
	return TRUE;
}

extern "C" _declspec(dllexport)
int M_Platorm_Dll_Win64_Main(HINSTANCE hInstance, IAppFactory& factory, cstr appName, HICON hLarge, HICON hSmall)
{
	return M_Platorm_Win64_Main(hInstance, factory, appName, hLarge, hSmall);
}

extern "C" _declspec(dllexport)
int M_Platorm_Dll_Win64_MainDirect(HINSTANCE hInstance, IDirectAppFactory& factory, cstr appName, HICON hLarge, HICON hSmall)
{
	return M_Platorm_Win64_MainDirect(hInstance, factory, appName, hLarge, hSmall);
}