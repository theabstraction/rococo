// mhost.os.win32.cpp : Defines the entry point for the application in Windows.
#include <rococo.mplat.dynamic.inl>
#include <sexy.lib.util.h>

#include "resource.h"

using namespace Rococo;

namespace MHost
{
	IDirectApp* CreateApp(Platform& platform, IDirectAppControl& control);
}

#ifdef _DEBUG
# define MPLAT_LIB MPLAT_DEBUG
#else
# define MPLAT_LIB MPLAT_RELEASE
#endif

int CALLBACK WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow)
{
	struct : IDirectAppFactory
	{
		IDirectApp* CreateApp(Platform& e, IDirectAppControl& control) override
		{
			return MHost::CreateApp(e, control);
		}
	} factory;

	return LoadPlatformDll_AndRun(hInstance, factory, "MPLAT - script host", MPLAT_LIB, nullptr, nullptr);
}