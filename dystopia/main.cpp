#include "dystopia.h"

#define WIN32_LEAN_AND_MEAN 
#define NOMINMAX
#include <windows.h>

#include "resource.h"

#include <rococo.renderer.h>
#include <rococo.dx11.renderer.win32.h>
#include <rococo.window.h>
#include "rococo.strings.h"

#include <vector>
#include <algorithm>

#ifdef _DEBUG
# pragma comment(lib, "dx11.renderer.debug.lib")
# pragma comment(lib, "rococo.maths.debug.lib")
# pragma comment(lib, "rococo.util.debug.lib")
# pragma comment(lib, "rococo.sexy.ide.debug.lib")
#else
# pragma comment(lib, "dx11.renderer.lib")
# pragma comment(lib, "rococo.maths.lib")
# pragma comment(lib, "rococo.util.lib")
# pragma comment(lib, "rococo.sexy.ide.lib")
#endif

#include <sexy.lib.s-parser.h>
#include <sexy.lib.util.h>
#include <sexy.lib.script.h>

#include <sexy.script.h>

using namespace Rococo;
using namespace Rococo::Windows;

namespace Dystopia
{
	IDystopiaApp* CreateDystopiaApp(IRenderer& renderer, IInstallation& installation);
}

namespace Rococo
{
	IOSSupervisor* GetOS();
}

int CALLBACK WinMain(HINSTANCE _hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow)
{
	using namespace Dystopia;

	HANDLE hInstanceLock = CreateEvent(nullptr, TRUE, FALSE, L"Dystopia_InstanceLock");

	if (GetLastError() == ERROR_ALREADY_EXISTS)
	{
		SetEvent(hInstanceLock);

		if (IsDebuggerPresent())
		{
			MessageBox(nullptr, L"Dystopia is already running", L"Dystopia Exception", MB_ICONEXCLAMATION);
		}
		return -1;
	}

	try
	{
		InitRococoWindows(_hInstance, LoadIcon(_hInstance, MAKEINTRESOURCE(IDI_ICON1)), LoadIcon(_hInstance, MAKEINTRESOURCE(IDI_ICON1)), nullptr, nullptr); // This must be called once, in WinMain or DllMain

		AutoFree<IOSSupervisor> os = GetOS();
		AutoFree<IInstallationSupervisor> installation = CreateInstallation(L"content.indicator.txt", *os);
		os->Monitor(installation->Content());

		rchar srcpath[_MAX_PATH];
		SecureFormat(srcpath, L"%sscripts\\native\\", installation->Content());

      Rococo::Script::SetDefaultNativeSourcePath(srcpath);

      AutoFree<IDX11Window> mainWindow(CreateDX11Window(*installation));
      SetWindowText(mainWindow->Window(), L"Dystopia");
      AutoFree<IDystopiaApp> app = Dystopia::CreateDystopiaApp(mainWindow->Renderer(), *installation);
      app->OnCreate();
      mainWindow->Run(hInstanceLock, *app);
	}
	catch (IException& ex)
	{
		OS::ShowErrorBox(NoParent(), ex, L"Dystopia threw an exception");
	}

	CloseHandle(hInstanceLock);

	return 0;
}


