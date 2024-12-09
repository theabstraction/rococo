#pragma once

#include <rococo.os.win32.h>
#include <rococo.mplat.h>
#include <mplat.to.app.events.inl>
#include <rococo.window.h>
#include <rococo.strings.h>

namespace Rococo
{
	using namespace Rococo::Strings;
	using namespace Rococo::Windows;

	const char* const MPLAT_DLL = "rococo.mplat.dynamic.dll";

	typedef int(*FN_M_Platorm_Dll_Win64_Main)(HINSTANCE hInstance, IAppFactory& factory, const char* appName, HICON hLarge, HICON hSmall);
	typedef int(*FN_M_Platorm_Dll_Win64_MainDirect)(HINSTANCE hInstance, IDirectAppFactory& factory, const char* appName, HICON hLarge, HICON hSmall);

	int LoadPlatformDll_AndRun(HINSTANCE hInstance, IAppFactory& factory, const char* appName, const char* moduleName, HICON hLarge, HICON hSmall)
	{
		auto module = LoadLibraryA(moduleName);
		if (module == nullptr)
		{
			int err = GetLastError();
			MessageBoxA(nullptr, "Could not load Dll", moduleName, MB_ICONERROR);
			return err;
		}

		auto addr = GetProcAddress(module, "M_Platorm_Dll_Win64_Main");
		if (addr == nullptr)
		{
			int err = GetLastError();
			MessageBoxA(nullptr, "Could not find entrypoint M_Platorm_Dll_Win64_Main", moduleName, MB_ICONERROR);
			return err;
		}

		auto run = (FN_M_Platorm_Dll_Win64_Main)addr;

		int retValue = run(hInstance, factory, appName, hLarge, hSmall);

		FreeLibrary(module);

		return retValue;
	}

	int LoadPlatformDll_AndRun(HINSTANCE hInstance, IDirectAppFactory& factory, const char* appName, const char* moduleName, HICON hLarge, HICON hSmall)
	{
		auto module = LoadLibraryA(moduleName);
		if (module == nullptr)
		{
			int err = GetLastError();

			char msg[256];
			OS::FormatErrorMessage(msg, sizeof msg, err);
			char fullMsg[256];
			SafeFormat(fullMsg, "Could not load Dll: Error %s", msg);
			MessageBoxA(nullptr, fullMsg, moduleName, MB_ICONERROR);
			return err;
		}

		auto addr = GetProcAddress(module, "M_Platorm_Dll_Win64_MainDirect");
		if (addr == nullptr)
		{
			int err = GetLastError();
			MessageBoxA(nullptr, "Could not find entrypoint M_Platorm_Dll_Win64_MainDirect", moduleName, MB_ICONERROR);
			return err;
		}

		auto run = (FN_M_Platorm_Dll_Win64_MainDirect)addr;

		int retValue = run(hInstance, factory, appName, hLarge, hSmall);

		FreeLibrary(module);

		return retValue;
	}
}

