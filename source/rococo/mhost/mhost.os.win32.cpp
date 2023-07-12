// mhost.os.win32.cpp : Defines the entry point for the application in Windows.
#include <rococo.mplat.dynamic.inl>
#include <rococo.strings.h>

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

int CALLBACK WinMain(_In_ HINSTANCE hInstance, _In_opt_ HINSTANCE hPrevInstance, _In_ LPSTR lpCmdLine,_In_ int nShowCmd)
{
	UNUSED(hPrevInstance);
	UNUSED(nShowCmd);

	char titleBuffer[128];
	Rococo::Strings::CLI::GetCommandLineArgument("-title:"_fstring, lpCmdLine, titleBuffer, sizeof titleBuffer, "Rococo MHost");

	struct : IDirectAppFactory
	{
		IDirectApp* CreateApp(Platform& e, IDirectAppControl& control) override
		{
			return MHost::CreateApp(e, control, GetCommandLineA());
		}
	} factory;

	return LoadPlatformDll_AndRun(hInstance, factory, titleBuffer, MPLAT_DLL, nullptr, nullptr);
}