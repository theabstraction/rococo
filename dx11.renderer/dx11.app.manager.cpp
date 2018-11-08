#include <rococo.os.win32.h>

#include <rococo.api.h>
#include <rococo.dx11.renderer.win32.h>

#include <rococo.mplat.h>

#include <rococo.ui.h>

#include <rococo.window.h>

#include "rococo.dx11.api.h"

namespace ANON
{
	using namespace Rococo;

	struct UltraClock : public IUltraClock
	{
		OS::ticks start;
		OS::ticks frameStart;
		OS::ticks frameDelta;
		Seconds dt;

		virtual OS::ticks FrameStart() const
		{
			return frameStart;
		}

		virtual OS::ticks Start() const
		{
			return start;
		}

		virtual OS::ticks FrameDelta() const
		{
			return frameDelta;
		}

		virtual Seconds DT() const
		{
			return dt;
		}
	};

	void MainLoop(HWND hWnd, HANDLE hInstanceLock, IApp& app)
	{
		UltraClock uc;
		OS::ticks lastTick = uc.start;
		OS::ticks frameCost = 0;

		float hz = (float)OS::CpuHz();

		uint32 sleepMS = 5;
		MSG msg = { 0 };
		while (msg.message != WM_QUIT && OS::IsRunning())
		{
			int64 msCost = frameCost / (OS::CpuHz() / 1000);

			int64 iSleepMS = sleepMS - msCost;

			if (iSleepMS < 0)
			{
				sleepMS = 0;
			}
			else if (sleepMS > (uint32) iSleepMS)
			{
				sleepMS = (uint32)iSleepMS;
			}
			DWORD status = MsgWaitForMultipleObjectsEx(1, &hInstanceLock, sleepMS, QS_ALLEVENTS, MWMO_ALERTABLE);

			OS::ticks now = OS::CpuTicks();

			if (status == WAIT_OBJECT_0)
			{
				ResetEvent(hInstanceLock);
				SetForegroundWindow(hWnd);
			}

			while (PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE))
			{
				TranslateMessage(&msg);
				DispatchMessageA(&msg);

				if (msg.message == WM_QUIT)
				{
					return;
				}
			}

			RECT rect;
			GetClientRect(hWnd, &rect);

			if ((rect.right - rect.left) == 0)
			{
				sleepMS = 1000;
				continue;
			}

			uc.frameStart = OS::CpuTicks();

			uc.frameDelta = uc.frameStart - lastTick;

			float dt0 = uc.frameDelta / (float)hz;
			dt0 = max(0.0f, dt0);
			dt0 = min(dt0, 0.05f);
			uc.dt = Seconds{ dt0 };

			sleepMS = app.OnFrameUpdated(uc);

			frameCost = OS::CpuTicks() - now;

			lastTick = uc.frameStart;
		}
	}

	struct AppManager : public IAppManager, public IEventCallback<SysUnstableArgs>, public IAppEventHandler
	{
		IApp& app;
		IDX11GraphicsWindow& window;

		AppManager(IDX11GraphicsWindow& _window, IApp& _app) : app(_app), window(_window)
		{
		}

		void OnEvent(SysUnstableArgs& unstable)
		{
			window.Renderer().SwitchToWindowMode();
		}

		void OnKeyboardEvent(const RAWKEYBOARD& k)
		{
			app.OnKeyboardEvent((const KeyboardEvent&)k);
		}

		void OnMouseEvent(const RAWMOUSE& m)
		{
			MouseEvent me;
			memcpy(&me, &m, sizeof(m));

			POINT p;
			GetCursorPos(&p);
			ScreenToClient(window.Window(), &p);

			me.cursorPos.x = p.x;
			me.cursorPos.y = p.y;

			app.OnMouseEvent(me);
		}

		void Run(HANDLE hInstanceLock, IApp& app) override
		{
			window.CaptureEvents(this);
			MainLoop(window.Window(), hInstanceLock, app);
			window.CaptureEvents(nullptr);
		}

		void Free() override
		{
			delete this;
		}

		void OnWindowClose() override
		{
			char text[256];
			GetWindowTextA(window.Window(), text, 255);
			text[255] = 0;

			AutoFree<DX11::ICountdownConfirmationDialog> confirmDialog(DX11::CreateCountdownConfirmationDialog());
			if (IDOK == confirmDialog->DoModal(window.Window(), text, "Quitting application", 8))
			{
				PostQuitMessage(0);
			}
		}
	};
}

namespace Rococo
{
	IAppManager* CreateAppManager(IDX11GraphicsWindow& window, IApp& app)
	{
		return new ANON::AppManager(window, app);
	}
}