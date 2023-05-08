// mhost.os.win32.cpp : Defines the entry point for the application in Windows.
#include <rococo.mplat.dynamic.inl>
#include <sexy.lib.script.h>
#include <sexy.lib.sexy-util.h>

#include "resource.h"

using namespace Rococo;

namespace MHost
{
	IDirectApp* CreateApp(Platform& platform, IDirectAppControl& control, cstr commandLine);

	namespace UI
	{
		void CaptureMouse(Rococo::Windows::IWindow& window)
		{
			::SetCapture(window);
		}

		void ReleaseMouse()
		{
			::ReleaseCapture();
		}
	}
}

#ifdef _DEBUG
# define MPLAT_LIB MPLAT_DEBUG
#else
# define MPLAT_LIB MPLAT_RELEASE
#endif

int CALLBACK WinMain(_In_ HINSTANCE hInstance, _In_opt_ HINSTANCE hPrevInstance, _In_ LPSTR lpCmdLine,_In_ int nShowCmd)
{
	struct : IDirectAppFactory
	{
		IDirectApp* CreateApp(Platform& e, IDirectAppControl& control) override
		{
			return MHost::CreateApp(e, control, GetCommandLineA());
		}
	} factory;

	return LoadPlatformDll_AndRun(hInstance, factory, "MPLAT - script host", MPLAT_LIB, nullptr, nullptr);
}