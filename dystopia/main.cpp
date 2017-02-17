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
# pragma comment(lib, "rococo.os.win32.debug.lib")
# pragma comment(lib, "rococo.maths.debug.lib")
# pragma comment(lib, "rococo.util.debug.lib")
# pragma comment(lib, "rococo.sexy.ide.debug.lib")
#else
# pragma comment(lib, "dx11.renderer.lib")
# pragma comment(lib, "rococo.os.win32.lib")
# pragma comment(lib, "rococo.maths.lib")
# pragma comment(lib, "rococo.util.lib")
# pragma comment(lib, "rococo.sexy.ide.lib")
#endif

#include <sexy.lib.s-parser.h>
#include <sexy.lib.util.h>
#include <sexy.lib.script.h>

using namespace Rococo;
using namespace Rococo::Windows;

namespace Dystopia
{
	IApp* CreateDystopiaApp(IRenderer& renderer, IInstallation& installation);
}

namespace Rococo
{
	IOSSupervisor* GetWin32OS(HINSTANCE hAppInstance);
}

struct FileHandle
{
	HANDLE hFile;

	FileHandle(HANDLE _hFile) : hFile(_hFile)
	{
	}

	bool IsValid() const
	{
		return hFile != INVALID_HANDLE_VALUE;
	}

	~FileHandle()
	{
		if (IsValid()) CloseHandle(hFile);
	}

	operator HANDLE()
	{
		return hFile;
	}
};

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

		AutoFree<IOSSupervisor> os = GetWin32OS(_hInstance);
		AutoFree<IInstallationSupervisor> installation = CreateInstallation(L"content.indicator.txt", *os);
		os->Monitor(installation->Content());

		wchar_t srcpath[_MAX_PATH];
		SecureFormat(srcpath, L"%sscripts\\native\\", installation->Content());
		SetEnvironmentVariable(L"SEXY_NATIVE_SRC_DIR", srcpath);

		struct : IAppFactory
		{
			IInstallation* installation;

			virtual IApp* CreateApp(IRenderer& renderer)
			{
				return Dystopia::CreateDystopiaApp(renderer, *installation);
			}
		} factory;

		factory.installation = installation;

		RendererMain(hInstanceLock, *installation, factory);
	}
	catch (IException& ex)
	{
		ShowErrorBox(NoParent(), ex, L"Dystopia threw an exception");
	}

	CloseHandle(hInstanceLock);

	return 0;
}


