#include <rococo.os.win32.h>
#include <rococo.api.h>
#include <rococo.dx11.renderer.win32.h>
#include <rococo.renderer.h>
#include <rococo.ui.h>
#include <rococo.window.h>
#include "rococo.dx11.api.h"
#include <rococo.ringbuffer.h>
#include <rococo.io.h>
#include <rococo.app.h>
#include <rococo.clock.h>
#include <rococo.os.h>

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

	bool RouteSysMessages(HWND hInstanceWnd, HANDLE hInstanceLock, DWORD sleepMS)
	{
		DWORD status = MsgWaitForMultipleObjectsEx(1, &hInstanceLock, sleepMS, QS_ALLEVENTS, MWMO_ALERTABLE);

		if (status == WAIT_OBJECT_0)
		{
			ResetEvent(hInstanceLock);
			SetForegroundWindow(hInstanceWnd);
		}

		MSG msg;
		while (PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE))
		{
			TranslateMessage(&msg);
			DispatchMessageA(&msg);

			if (msg.message == WM_QUIT)
			{
				return false;
			}
		}

		return true;
	}

	void MainLoop(HWND hWnd, HANDLE hInstanceLock, IApp& app, OS::IAppControl& appControl)
	{
		UltraClock uc;
		OS::ticks lastTick = uc.start;
		OS::ticks frameCost = 0;

		float hz = (float)OS::CpuHz();

		uint32 sleepMS = 5;
		MSG msg = { 0 };
		while (msg.message != WM_QUIT && appControl.IsRunning())
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

			if (!RouteSysMessages(hWnd, hInstanceLock, sleepMS))
			{
				return;
			}

			OS::ticks now = OS::CpuTicks();

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

	// See IAppManager for an explanation of this class
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

		static int32 VcodeToUnicode(int32 virtualKeyCode, int32 scancode, HKL layout)
		{
			BYTE keystate[256];
			GetKeyboardState(keystate);

			WCHAR buffer[4] = { 0,0,0,0 };
			UINT flags = 0;
			int charsRead = ToUnicodeEx(virtualKeyCode, scancode, keystate, buffer, 4, flags, layout);
			return (charsRead == 1) ? buffer[0] : 0;
		}

		void OnKeyboardEvent(const RAWKEYBOARD& k, HKL hKeyboardLayout)
		{
			KeyboardEvent key;
			((RAWKEYBOARD&)key) = k;
			key.unicode = VcodeToUnicode(k.VKey, key.scanCode, hKeyboardLayout);
			app.OnKeyboardEvent(key);
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

		void Run(HANDLE hInstanceLock, IApp& app, OS::IAppControl& appControl) override
		{
			window.CaptureEvents(this);
			MainLoop(window.Window(), hInstanceLock, app, appControl);
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

	// See IDirectAppManager for an explanation of this class
	struct DirectAppManager :
		public IDirectAppManager,
		public IEventCallback<SysUnstableArgs>,
		public IAppEventHandler,
		public IDirectAppControl
	{
		AutoFree<IDirectApp> app;
		IDX11GraphicsWindow& window;
		OneReaderOneWriterCircleBuffer<KeyboardEvent> keyBuffer;
		OneReaderOneWriterCircleBuffer<MouseEvent> mouseBuffer;
		HANDLE hInstanceLock;
		Vec2 mouseDelta = { 0,0 };

		DirectAppManager(Platform& platform, IDX11GraphicsWindow& _window, IDirectAppFactory& _factory) : 
			window(_window),
			keyBuffer(16),
			mouseBuffer(16)
		{
			app = _factory.CreateApp(platform, *this);
		}

		void OnEvent(SysUnstableArgs& unstable) override
		{
			window.Renderer().SwitchToWindowMode();
		}

		static int32 VcodeToUnicode(int32 virtualKeyCode, int32 scancode, HKL layout)
		{
			BYTE keystate[256];
			GetKeyboardState(keystate);

			WCHAR buffer[4] = { 0,0,0,0 };
			UINT flags = 0;
			int charsRead = ToUnicodeEx(virtualKeyCode, scancode, keystate, buffer, 4, flags, layout);
			return (charsRead == 1) ? buffer[0] : 0;
		}

		void OnKeyboardEvent(const RAWKEYBOARD& rawKey, HKL hKeyboardLayout) override
		{
			static_assert(sizeof(KeyboardEvent) == sizeof(RAWKEYBOARD) + sizeof(int32), "Bad Keyboard Event size");

			KeyboardEvent ke;
			((RAWKEYBOARD&)(ke)) = rawKey;
			ke.unicode = VcodeToUnicode(ke.VKey, ke.scanCode, hKeyboardLayout);

			auto* b = keyBuffer.GetBackSlot();
			if (b) *b = ke;
			keyBuffer.WriteBack();
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

			auto* b = mouseBuffer.GetBackSlot();
			if (b) *b = me;
			mouseBuffer.WriteBack();

			if (m.usFlags == MOUSE_MOVE_RELATIVE)
			{
				mouseDelta.x += (float) m.lLastX;
				mouseDelta.y += (float) m.lLastY;
			}
		}

		bool TryGetNextKeyboardEvent(KeyboardEvent& k) override
		{
			return keyBuffer.TryPopFront(k);
		}

		bool TryGetNextMouseEvent(MouseEvent& m) override
		{
			return mouseBuffer.TryPopFront(m);
		}

		void GetNextMouseDelta(Vec2& delta) override
		{
			delta = { fmodf(mouseDelta.x, 16384.0f), fmodf(mouseDelta.y, 16384.0f) };
			mouseDelta = { 0,0 };
		}

		bool TryRouteSysMessages(uint32 sleepMS) override
		{
			return RouteSysMessages(window.Window(), hInstanceLock, sleepMS);
		}

		void Run(HANDLE hInstanceLock) override
		{
			this->hInstanceLock = hInstanceLock;

			window.CaptureEvents(this);
			app->Run();
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

	IDirectAppManager* CreateAppManager(Platform& platform, IDX11GraphicsWindow& window, IDirectAppFactory& factory)
	{
		return new ANON::DirectAppManager(platform, window, factory);
	}
}