#pragma once

#include <rococo.os.win32.h>
#include <rococo.mplat.h>

#ifdef _DEBUG
# pragma comment(lib, "rococo.util.debug.lib")
# pragma comment(lib, "rococo.maths.debug.lib")
#else
# pragma comment(lib, "rococo.util.lib")
# pragma comment(lib, "rococo.maths.lib")
#endif

#include <rococo.window.h>

namespace Rococo
{
	using namespace Rococo::Windows;

	const char* const MPLAT_RELEASE = "rococo.mplat.dynamic.dll";
	const char* const MPLAT_DEBUG = "rococo.mplat.dynamic.debug.dll";

	typedef int(*FN_M_Platorm_Dll_Win64_Main)(HINSTANCE hInstance, IAppFactory& factory, const char* appName, HICON hLarge, HICON hSmall);

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
			MessageBoxA(nullptr, "Could not find entrypoint in M_Platorm_Dll_Win64_Main", moduleName, MB_ICONERROR);
			return err;
		}

		auto run = (FN_M_Platorm_Dll_Win64_Main)addr;

		int retValue = run(hInstance, factory, appName, hLarge, hSmall);

		FreeLibrary(module);

		return retValue;
	}

	using namespace Rococo::Events;

	Events::EventId UIInvoke::EvId()
	{
		static EventId invokeEvent = "ui.invoke"_event;
		return invokeEvent;
	}

	namespace Events
	{
		EventId BuysEventId = "mplat.busy"_event;
	}
}

