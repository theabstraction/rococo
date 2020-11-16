#include <rococo.api.h>
#include <rococo.os.win32.h>
#include <rococo.libs.inl>
#include <dxgi.h>
#include <rococo.window.h>
#include <rococo.strings.h>
#include <rococo.os.h>
#include <rococo.renderer.h>
#include <rococo.strings.h>
#include <vector>
#include <rococo.mplat.h>
#include <rococo.fonts.hq.h>

#include <sexy.lib.s-parser.h>
#include <sexy.lib.util.h>
#include <sexy.lib.script.h>

#ifdef _DEBUG
# pragma comment(lib, "rococo.windows.debug.lib")
# pragma comment(lib, "rococo.fonts.debug.lib")
# pragma comment(lib, "rococo.mplat.debug.lib")
# pragma comment(lib, "rococo.sexy.ide.debug.lib")
# pragma comment(lib, "dx11.deferred.renderer.debug.lib")
#else
# pragma comment(lib, "rococo.windows.lib")
# pragma comment(lib, "rococo.fonts.lib")
# pragma comment(lib, "rococo.mplat.lib")
# pragma comment(lib, "rococo.sexy.ide.lib")
# pragma comment(lib, "dx11.deferred.renderer.lib")
#endif

#include "..\dx11.deferred.renderer\rococo.dx11.h"

#pragma comment(lib, "dxguid.lib")
#pragma comment(lib, "dxgi.lib")
#pragma comment(lib, "d3d11.lib")
#pragma comment(lib, "D3DCompiler.lib")

using namespace Rococo;
using namespace Rococo::Graphics;

void Main(HINSTANCE hInstance)
{
	AutoFree<IOSSupervisor> os = GetOS();
	AutoFree<IInstallationSupervisor> installation = CreateInstallation(L"content.indicator.txt", *os);
	AutoFree<IWin32AdapterContext> ac = CreateWin32AdapterContext(0, 0);
	AutoFree<IDX11System> dx11 = CreateDX11System(ac->AC(), *installation);

	struct CONTEXT : IDX1RendererWindowEventHandler
	{
		bool isRunning = true;
		int err = 0;
		HString msg;

		void OnCloseRequested(IDX11Window& window) override
		{
			isRunning = false;
		}

		void OnMessageQueueException(IDX11Window& window, IException& ex) override
		{
			err = ex.ErrorCode();
			msg = ex.Message();
		}

		void OnKeyboardEvent(const RAWKEYBOARD& k, HKL hKeyboardLayout) override
		{

		}

		void OnMouseEvent(const RAWMOUSE& m) override
		{

		}

		void ThrowOnError()
		{
			if (err != 0 || msg.length() != 0)
			{
				Throw(err, msg);
			}
		}
	} app;

	DX11WindowContext wc{ Windows::NoParent(), app, {800,600}, "DX11 Test Window", hInstance };
	AutoFree<IDX11Window> window = dx11->CreateDX11Window(wc);

	auto start = Rococo::OS::CpuTicks();

	MSG msg;
	while (app.isRunning)
	{
		MsgWaitForMultipleObjects(0, NULL, FALSE, 100, QS_ALLINPUT);

		while (PeekMessageA(&msg, nullptr, 0, 0, PM_REMOVE))
		{
			TranslateMessage(&msg);
			DispatchMessageA(&msg);
		}

		Rococo::OS::ticks now = Rococo::OS::CpuTicks();

		enum { TIME_OUT_SECONDS = 120 };
		if (now - start > Rococo::OS::CpuHz() * TIME_OUT_SECONDS)
		{
			break;
		}
	}

	app.ThrowOnError();
}

int CALLBACK WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow)
{
	try
	{
		Main(hInstance);
		return 0;
	}
	catch (IException& ex)
	{
		Rococo::OS::ShowErrorBox(Rococo::Windows::NoParent(), ex, "DX11 Deferred Renderer Test App");
		return ex.ErrorCode();
	}
}
