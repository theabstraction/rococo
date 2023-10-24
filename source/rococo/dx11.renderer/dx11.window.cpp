#include "dx11.renderer.h"
#include <rococo.io.h>
#include <rococo.os.win32.h>
#include <rococo.win32.rendering.h>

#include "dx11helpers.inl"

#include <dxgi.h>
#include <d3d11.h>

#include "dx11.factory.h"
#include "dx11.renderer.h"

#include <vector>

#include <rococo.window.h>
#include <rococo.renderer.h>

#include "dx11.imports.inl"

#pragma comment(lib, "lib-tiff.lib" )
#pragma comment(lib, "lib-jpg.lib" )

namespace Rococo
{
	namespace DX11
	{
		struct WindowStartupParams
		{
			cstr title;
		};
	}
}

namespace ANON
{
	using namespace Rococo;

	void RegisterRawInput(HWND hWnd)
	{
		RAWINPUTDEVICE mouseDesc;
		mouseDesc.hwndTarget = hWnd;
		mouseDesc.dwFlags = 0;
		mouseDesc.usUsage = 0x02;
		mouseDesc.usUsagePage = 0x01;
		if (!RegisterRawInputDevices(&mouseDesc, 1, sizeof(mouseDesc)))
		{
			Throw(GetLastError(), "RegisterRawInputDevices(&mouseDesc, 1, sizeof(mouseDesc) failed");
		}

		RAWINPUTDEVICE keyboardDesc;
		keyboardDesc.hwndTarget = hWnd;
		keyboardDesc.dwFlags = 0;
		keyboardDesc.usUsage = 0x06;
		keyboardDesc.usUsagePage = 0x01;
		if (!RegisterRawInputDevices(&keyboardDesc, 1, sizeof(keyboardDesc)))
		{
			Throw(GetLastError(), "RegisterRawInputDevices(&keyboardDesc, 1, sizeof(keyboardDesc)) failed");
		}
	}

	class DX11GraphicsWindow : public IGraphicsWindow, public Windows::IWindow
	{
		DX11::Factory factory; // Yes, a value type, not a reference
		HWND hWnd;
		WindowSpec ws;
		IAppEventHandler* handler = nullptr;
		AutoFree<IExpandingBuffer> eventBuffer;
		HKL keyboardLayout = nullptr;
		bool hasFocus = true;
		Rococo::DX11::IDX11Renderer& renderer;
		bool linkedToDX11Controls;

		AutoFree<DX11::IDX11WindowBacking> backing;

		void CaptureEvents(IAppEventHandler* handler)
		{
			this->handler = handler;
		}

		LRESULT RouteInput(HWND hWnd, WPARAM wParam, LPARAM lParam)
		{
			UINT sizeofBuffer = 0;
			if (NO_ERROR != GetRawInputData((HRAWINPUT)lParam, RID_INPUT, NULL, &sizeofBuffer, sizeof(RAWINPUTHEADER)))
			{
				return DefWindowProc(hWnd, WM_INPUT, wParam, lParam);
			}

			eventBuffer->Resize(sizeofBuffer);

			char* buffer = (char*)eventBuffer->GetData();

			if (GetRawInputData((HRAWINPUT)lParam, RID_INPUT, buffer, &sizeofBuffer, sizeof(RAWINPUTHEADER)) == (UINT)-1)
			{
				return DefWindowProc(hWnd, WM_INPUT, wParam, lParam);
			}

			RAWINPUT& raw = *((RAWINPUT*)buffer);
			RAWINPUT* pRaw = &raw;

			if (hasFocus)
			{
				if (raw.header.dwType == RIM_TYPEMOUSE)
				{
					if (handler) handler->OnMouseEvent(raw.data.mouse);
				}
				else if (raw.header.dwType == RIM_TYPEKEYBOARD)
				{
					if (handler) handler->OnKeyboardEvent(raw.data.keyboard, keyboardLayout);
				}

				return 0;
			}
			else
			{
				return DefRawInputProc(&pRaw, 1, sizeof(RAWINPUTHEADER));
			}
		}

		LRESULT OnMessageProtected(UINT uMsg, WPARAM wParam, LPARAM lParam)
		{
			if (ws.messageSink)
			{
				LRESULT result = 0;
				if (ws.messageSink->InterceptMessage(result, hWnd, uMsg, wParam, lParam))
				{
					return result;
				}
			}

			switch (uMsg)
			{
			case WM_ACTIVATE:
				{
					auto state = 0xFFFF & wParam;

					// state 0 => WA_INACTIVE // inactive
					// state 1 => WA_ACTIVE  // activated by something other than a mouse click
					// state 2 => WA_CLICKACTIVE // activated by a mouse click

					if (state != 0)
					{
						if (linkedToDX11Controls) VALIDATEDX11(factory.factory.MakeWindowAssociation(hWnd, 0));
					}
					else
					{
						if (linkedToDX11Controls)  VALIDATEDX11(factory.factory.MakeWindowAssociation(nullptr, 0));
					}
					return 0;
				}
			case WM_SETCURSOR:
				return TRUE;
			case WM_SETFOCUS:
				hasFocus = true;
				RegisterRawInput(hWnd);
				return 0L;
			case WM_KILLFOCUS:
				hasFocus = false;
				return 0L;
			case WM_GETMINMAXINFO:
				{
					auto* m = (MINMAXINFO*)lParam;
					m->ptMinTrackSize = POINT{ ws.minSpan.x, ws.minSpan.y };
				}
				return 0L;
			case WM_INPUTLANGCHANGE:
				keyboardLayout = GetKeyboardLayout(0);
				return 0L;
			case WM_SIZE:
				renderer.OnWindowResized(*backing, Vec2i{ LOWORD(lParam), HIWORD(lParam) } );
				return 0L;
			case WM_INPUT:
				return RouteInput(hWnd, wParam, lParam);
			case WM_CLOSE:
				if (handler)
				{
					handler->OnWindowClose();
					return 0L;
				}
				else
				{
					break;
				}
			}

			return DefWindowProcA(hWnd, uMsg, wParam, lParam);
		}

		static LRESULT OnMessage(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
		{
			auto* This = (DX11GraphicsWindow*) GetWindowLongPtrA(hWnd, GWLP_USERDATA);

			try
			{
				return This->OnMessageProtected(uMsg, wParam, lParam);
			}
			catch (IException& ex)
			{
				This->factory.logger.OnMessageException(ex, uMsg);
				return DefWindowProcA(hWnd, uMsg, wParam, lParam);
			}
		}
	public:
		DX11GraphicsWindow(DX11::Factory& _factory, Rococo::DX11::IDX11Renderer& _renderer, ATOM windowsClass, const WindowSpec& _ws, bool _linkedToDX11Controls):
			factory(_factory), renderer(_renderer), ws(_ws), eventBuffer(CreateExpandingBuffer(128)), linkedToDX11Controls(_linkedToDX11Controls)
		{
			hWnd = CreateWindowExA(
				ws.exStyle,
				(cstr)windowsClass,
				"Rococo DX11 Graphics Window",
				ws.style,
				ws.X,
				ws.Y,
				ws.Width,
				ws.Height,
				ws.hParentWnd,
				nullptr,
				ws.hInstance,
				nullptr
			);

			if (hWnd == nullptr)
			{
				Throw(GetLastError(), "DX11GraphicsWindow::CreateWindowExA failed");
			}

			keyboardLayout = GetKeyboardLayout(0);

			SetWindowLongPtrA(hWnd, GWLP_USERDATA, (LONG_PTR) this);

			backing = DX11::CreateDX11WindowBacking(factory.device, factory.dc, hWnd, factory.factory, static_cast<DX11::IDX11TextureManager&>(renderer.Textures()));
		}

		void PostConstruct()
		{
			ShowWindow(hWnd, SW_SHOW);

			// Flush messages and allow our window to stabilize

			MSG msg;
			while (PeekMessageA(&msg, nullptr, 0, 0, PM_REMOVE))
			{
				if (msg.message == WM_QUIT)
				{
					return;
				}
				TranslateMessage(&msg);
				DispatchMessageA(&msg);
			}

			SetWindowLongPtrA(hWnd, GWLP_WNDPROC, (LONG_PTR) OnMessage);

			if (linkedToDX11Controls) factory.factory.MakeWindowAssociation(hWnd, 0);
			
			RegisterRawInput(hWnd);
		}

		virtual ~DX11GraphicsWindow()
		{
			if (hWnd)
			{
				renderer.SwitchToWindowMode();
				DestroyWindow(hWnd);
				hWnd = nullptr;
			}
		}

		void MakeRenderTarget()
		{
			backing->ResetOutputBuffersForWindow( {0, 0});
			renderer.SetWindowBacking(backing);
		}

		Windows::IWindow& Window()
		{
			return *this;
		}

		operator HWND() const override
		{
			return hWnd;
		}

		void Free() override
		{
			delete this;
		}

		IRenderer& Renderer()
		{
			return renderer;
		}
	};
}

namespace Rococo
{
	namespace DX11
	{
		IGraphicsWindow* CreateDX11GraphicsWindow(DX11::Factory& factory, Rococo::DX11::IDX11Renderer& renderer, ATOM windowClass, const WindowSpec& ws, bool linkedToDX11Controls)
		{
			auto* g = new ANON::DX11GraphicsWindow(factory, renderer, windowClass, ws, linkedToDX11Controls);

			try
			{
				g->PostConstruct();
			}
			catch (IException&)
			{
				g->Free();
				throw;
			}

			return g;
		}
	}
}