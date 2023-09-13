#include <rococo.api.h>
#include <rococo.os.win32.h>
#include "rococo.dx11.h"
#include <rococo.auto-release.h>
#include <rococo.strings.h>
#include <rococo.window.h>

#include <d3d11_4.h>
#include <dxgi1_6.h>

#include <rococo.io.h>

#include <rococo.DirectX.h>

#include <vector>

#include <CommCtrl.h>

using namespace Rococo;
using namespace Rococo::Graphics;

namespace ANON
{
	ATOM dx11Atom = 0;
	char atomName[64];

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

	void RegisterWndHandler(HINSTANCE hResourceInstance)
	{
		if (dx11Atom != 0) return;

		enum { IDI_ICON1 = 101 };

		WNDCLASSEXA classDef = { 0 };
		classDef.cbSize = sizeof(classDef);
		classDef.style = 0;
		classDef.cbWndExtra = 0;
		classDef.hbrBackground = (HBRUSH)COLOR_WINDOW;
		classDef.hCursor = LoadCursor(nullptr, IDC_ARROW);
		classDef.hIcon = LoadIconA(hResourceInstance, (const char*)IDI_ICON1);
		classDef.hIconSm = LoadIconA(hResourceInstance, (const char*)IDI_ICON1);
		classDef.hInstance = hResourceInstance;

		SafeFormat(atomName, "Rococo-DX11-deferred-%llx", Rococo::Time::TickCount());
		classDef.lpszClassName = atomName;
		classDef.lpszMenuName = NULL;
		classDef.lpfnWndProc = DefWindowProcA;

		dx11Atom = RegisterClassExA(&classDef);

		if (dx11Atom == 0)
		{
			Throw(GetLastError(), "Error creating DX11 Window class atom");
		}
	}

	void GetSwapChainForWindow(HWND hWnd, DXGI_SWAP_CHAIN_DESC1& desc)
	{
		desc.AlphaMode = DXGI_ALPHA_MODE_IGNORE;
		desc.Width = 0;
		desc.Height = 0;
		desc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
		desc.Stereo = FALSE;
		desc.SampleDesc = { 1,0 };
		desc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
		desc.Scaling = DXGI_SCALING_NONE;
		desc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
		desc.Flags = 0;
		desc.BufferCount = 2;
	}

	void GetSwapChainForFullscreen(HWND hWnd, DXGI_SWAP_CHAIN_FULLSCREEN_DESC& desc)
	{
		desc.RefreshRate.Numerator = 60;
		desc.RefreshRate.Denominator = 1;
		desc.ScanlineOrdering = DXGI_MODE_SCANLINE_ORDER_UNSPECIFIED;
		desc.Scaling = DXGI_MODE_SCALING_CENTERED;
		desc.Windowed = TRUE;
	}


	HWND CreateWindowFromContext(DX11WindowContext& context)
	{
		DWORD exStyle;
		DWORD style;

		HWND hParentWnd = context.parent;

		if (!hParentWnd)
		{
			exStyle = WS_EX_APPWINDOW;
			style = WS_OVERLAPPEDWINDOW;
		}
		else
		{
			exStyle = 0;
			style = WS_CHILDWINDOW;
		}

		HWND hMainWnd = CreateWindowExA(
			exStyle,
			(const char*)dx11Atom,
			context.title,
			style,
			0,
			0,
			context.windowedSpan.x,
			context.windowedSpan.y,
			hParentWnd,
			nullptr,
			(HINSTANCE)context.hResourceInstance,
			nullptr
		);

		return hMainWnd;
	}

	class DX11Window: public IDX11Window, public Windows::IWindow, public IShaderViewGrabber
	{
		IDXGIFactory7& dxgiFactory;
		IDX11System& system;
		DX11WindowContext wc;
		HWND hMainWnd = nullptr;
		HFONT consoleFont = nullptr;
		uint64 frameCount = 0;
		bool hasFocus = false;
		HKL keyboardLayout = nullptr;
		AutoRelease<IDXGISwapChain4> swapChain;
		AutoFree<IExpandingBuffer> eventBuffer;
		IShaderCache* shaders = nullptr;
		ITextureSupervisor& textures;

		TextureId idBackBuffer;
		TextureId idDepthBuffer;

		HWND hReportWnd;
	public:
		DX11Window(IDX11System& ref_system, DX11WindowContext& ref_wc, ITextureSupervisor& ref_textures):
			dxgiFactory(ref_system.Factory()), system(ref_system), wc(ref_wc), 
			eventBuffer(CreateExpandingBuffer(128)), textures(ref_textures)
		{
			RegisterWndHandler((HINSTANCE)wc.hResourceInstance);
			hMainWnd = CreateWindowFromContext(wc);
			keyboardLayout = GetKeyboardLayout(0);

			auto& device = system.Device();

			try
			{
				DXGI_SWAP_CHAIN_DESC1 winDesc;
				GetSwapChainForWindow(hMainWnd, winDesc);
				DXGI_SWAP_CHAIN_FULLSCREEN_DESC fullDesc;
				GetSwapChainForFullscreen(hMainWnd, fullDesc);

				AutoRelease<IDXGISwapChain1> swapChain1;
				VALIDATE_HR(dxgiFactory.CreateSwapChainForHwnd(&device, hMainWnd, &winDesc, &fullDesc, NULL, &swapChain1));
				VALIDATE_HR(swapChain1->QueryInterface(&swapChain));

				AutoRelease<ID3D11Texture2D> backBuffer;
				VALIDATE_HR(swapChain->GetBuffer(0, __uuidof(ID3D11Texture2D), (void**)&backBuffer));

				ID3D11Texture2D1* back1 = nullptr;
				VALIDATE_HR(backBuffer->QueryInterface(&back1));

				char backbuffername[32];
				SafeFormat(backbuffername, "BackBuffer_H%X", hMainWnd);

				idBackBuffer = textures.AddTx2D_Direct(backbuffername, back1);
				
				Vec2i span = textures.GetSpan(idBackBuffer);

				char depthbuffername[32];
				SafeFormat(depthbuffername, "DepthBuffer_H%X", hMainWnd);
				idDepthBuffer = textures.AddDepthStencil(depthbuffername, span, 32, 0);

				LOGFONTA logFont = { 0 };
				SafeFormat(logFont.lfFaceName, "Consolas");
				logFont.lfHeight = -11;

				consoleFont = CreateFontIndirectA(&logFont);

				wc.evHandler.OnResizeBackBuffer(idBackBuffer, span);
			}
			catch (IException&)
			{
				DestroyWindow(hMainWnd);
				throw;
			}
		}

		virtual ~DX11Window()
		{
			if (hMainWnd) DestroyWindow(hMainWnd);

			if (consoleFont) DeleteObject((HGDIOBJ)consoleFont);
		}

		void ActivateCustomWindowsProcedureAndMakeVisible()
		{
			SetWindowLongPtrA(hMainWnd, GWLP_USERDATA, (LONG_PTR)this);
			SetWindowLongPtrA(hMainWnd, GWLP_WNDPROC, (LONG_PTR)OnMessage);
			ShowWindow(hMainWnd, SW_SHOW);

			auto hDesktop = GetDesktopWindow();
			RECT rect;
			GetClientRect(hDesktop, &rect);
			int32 width = rect.right - rect.left;

			DWORD exStyle = 0;
			DWORD style = WS_POPUP | WS_BORDER;
			hReportWnd = CreateWindowExA(exStyle, WC_STATICA, "DebugPopup", style, 20, 20, width-40, 240, hMainWnd,
				NULL, (HINSTANCE)wc.hResourceInstance, NULL);
		}

		void Free() override
		{
			delete this;
		}

		operator HWND() const override
		{
			return hMainWnd;
		}

		Rococo::Windows::IWindow& Window() override
		{
			return *this;
		}

		Vec2i Span() const override
		{
			RECT rect;
			GetClientRect(hMainWnd, &rect);
			return { rect.right, rect.bottom };
		}

		void WaitFrames(int nFrames, int msPerFrame)
		{
			uint64 releaseAt = frameCount + nFrames;

			while (releaseAt > frameCount)
			{
				MsgWaitForMultipleObjects(0, NULL, FALSE, msPerFrame, QS_ALLINPUT);

				InvalidateRect(hMainWnd, NULL, TRUE);

				MSG msg;
				while (PeekMessageA(&msg, nullptr, 0, 0, PM_REMOVE))
				{
					TranslateMessage(&msg);
					DispatchMessageA(&msg);
				}
			}
		}

		void OnActivate()
		{
			dxgiFactory.MakeWindowAssociation(hMainWnd, 0);
		}

		void OnDeactivate()
		{
			dxgiFactory.MakeWindowAssociation(nullptr, 0);
		}

		void OnSize(Vec2i span)
		{
			Vec2i backBufferSpan = textures.GetSpan(idBackBuffer);
			if (span != backBufferSpan)
			{
				// IDXGISwapChain::ResizeBuffers requires back buffer and depth buffer be released 

				DXGI_SWAP_CHAIN_DESC1 desc;
				GetSwapChainForWindow(hMainWnd, desc);
				textures.Release(idBackBuffer);
				textures.Release(idDepthBuffer);
				VALIDATE_HR(swapChain->ResizeBuffers(desc.BufferCount, 0, 0, desc.Format, desc.Flags));

				AutoRelease<ID3D11Texture2D> backBuffer;
				VALIDATE_HR(swapChain->GetBuffer(0, __uuidof(ID3D11Texture2D), (void**)&backBuffer));

				ID3D11Texture2D1* back1 = nullptr;
				VALIDATE_HR(backBuffer->QueryInterface(&back1));

				textures.SetTx2D_Direct(idBackBuffer, back1);
				textures.ResizeDepthStencil(idDepthBuffer, span);
			}
		}

		LRESULT RouteInput(HWND hWnd, WPARAM wParam, LPARAM lParam)
		{
			UINT sizeofBuffer;
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
					wc.evHandler.OnMouseEvent(raw.data.mouse);
				}
				else if (raw.header.dwType == RIM_TYPEKEYBOARD)
				{
					wc.evHandler.OnKeyboardEvent(raw.data.keyboard, keyboardLayout);
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
			switch (uMsg)
			{
			case WM_ACTIVATE:
			{
				auto state = 0xFFFF & wParam;
				if (state != 0)
				{
					OnActivate();
				}
				else
				{
					OnDeactivate();
				}
				return 0;
			}
			case WM_SETFOCUS:
				hasFocus = true;
				RegisterRawInput(hMainWnd);
				return 0L;
			case WM_KILLFOCUS:
				hasFocus = false;
				return 0L;
			case WM_GETMINMAXINFO:
			{
				auto* m = (MINMAXINFO*)lParam;
				m->ptMinTrackSize = POINT{ wc.windowedSpan.x, wc.windowedSpan.y };
			}
			return 0L;
			case WM_INPUTLANGCHANGE:
				keyboardLayout = GetKeyboardLayout(0);
				return 0L;
			case WM_SIZE:
				OnSize(Vec2i{ LOWORD(lParam), HIWORD(lParam) });
				return 0L;
			case WM_INPUT:
				return RouteInput(hMainWnd, wParam, lParam);
			case WM_CLOSE:
				wc.evHandler.OnCloseRequested(*this);
				return 0L;
			case WM_ERASEBKGND:
				return 0L;
			case WM_PAINT:
				{
					frameCount++;
					PAINTSTRUCT ps;
					HDC dc = BeginPaint(hMainWnd, &ps);
					EndPaint(hMainWnd, &ps);
					return 0L;
				}
			}

			return DefWindowProcA(hMainWnd, uMsg, wParam, lParam);
		}

		static LRESULT OnMessage(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
		{
			auto* This = (DX11Window*)GetWindowLongPtrA(hWnd, GWLP_USERDATA);

			try
			{
				return This->OnMessageProtected(uMsg, wParam, lParam);
			}
			catch (IException& ex)
			{
				This->wc.evHandler.OnMessageQueueException(*This, ex);
				return DefWindowProcA(hWnd, uMsg, wParam, lParam);
			}
		}

		void SwitchToFullscreenMode() override
		{
			BOOL isFullScreen;
			AutoRelease<IDXGIOutput> output;
			if SUCCEEDED(swapChain->GetFullscreenState(&isFullScreen, &output))
			{
				if (!isFullScreen)
				{
					swapChain->SetFullscreenState(true, nullptr);
				}
			}
		}

		void SwitchToWindowMode() override
		{
			BOOL isFullScreen;
			AutoRelease<IDXGIOutput> output;
			if SUCCEEDED(swapChain->GetFullscreenState(&isFullScreen, &output))
			{
				if (isFullScreen)
				{
					swapChain->SetFullscreenState(false, nullptr);
				}
			}
		}

		void ShowShaderErrors(HWND hWnd)
		{
			if (!badShaders.empty())
			{
				char buf[4096];
				StackStringBuilder sb(buf, sizeof buf);

				char windowTitle[1024];
				GetWindowTextA(hMainWnd, windowTitle, 1024);

				sb << windowTitle << "\n\n";

				int index = 1;

				for (auto& bad : badShaders)
				{
					char hrMsg[256];
					Rococo::OS::FormatErrorMessage(hrMsg, sizeof hrMsg, bad.hr);
					sb << index++ << ". " << bad.resource;
					sb.AppendFormat(": (%d / 0x%8.8X) %s\n", bad.hr, bad.hr, hrMsg);
					sb << bad.msg.c_str();
					sb << "\n\n";
				}

				SetWindowTextA(hWnd, buf);
				ShowWindow(hWnd, SW_SHOW);
			}
			else
			{
				SetWindowTextA(hWnd, "");
				ShowWindow(hWnd, SW_HIDE);
			}
		}


		void ShowWindowVenue(IMathsVisitor& visitor)
		{
			POINT pt;
			GetCursorPos(&pt);

			visitor.ShowString("Abs Mouse Cursor:", "(%d %d)", pt.x, pt.y);

			ScreenToClient(hMainWnd, &pt);

			visitor.ShowString("Rel Mouse Cursor:", "(%d %d)", pt.x, pt.y);

			visitor.ShowPointer("HANDLE", hMainWnd);
			RECT rect;
			GetClientRect(hMainWnd, &rect);
			visitor.ShowString("-> Client Rect", "(%d %d) to (%d %d)", rect.left, rect.top, rect.right, rect.bottom);

			GetWindowRect(hMainWnd, &rect);
			visitor.ShowString("-> Window Rect", "(%d %d) to (%d %d)", rect.left, rect.top, rect.right, rect.bottom);

			LONG x = GetWindowLongA(hMainWnd, GWL_STYLE);
			visitor.ShowString("-> GWL_STYLE", "0x%8.8X", x);

			x = GetWindowLongA(hMainWnd, GWL_EXSTYLE);
			visitor.ShowString("-> GWL_EXSTYLE", "0x%8.8X", x);

			HWND hParent = GetParent(hMainWnd);
			if (hParent)	visitor.ShowPointer("-> Parent", hParent);
			else			visitor.ShowString("-> Parent", "None (top-level window)");
		}

		void MonitorShaderErrors(IShaderCache* shaders) override
		{
			this->shaders = shaders;
		}

		struct BadShader
		{
			HString msg;
			HString resource;
			HRESULT hr;
			ShaderId id;
		};

		std::vector<BadShader> badShaders;

		void OnGrab(const ShaderView& s)
		{
			if FAILED(s.hr)
			{
				for (auto& t : badShaders)
				{
					if (t.id == s.id)
					{
						t.hr = s.hr;
						t.msg = s.errorString;
						t.resource = s.resourceName;
						return;
					}
				}
				badShaders.push_back({ s.errorString, s.resourceName, s.hr, s.id });
			}
		}

		bool UpdateShaderState()
		{
			int updateCount = shaders->TryGrabAndPopNextError(*this) ? 1 : 0;

			auto* pShaders = this->shaders;

			auto i = std::remove_if(badShaders.begin(), badShaders.end(),
				[pShaders](BadShader& bad)
				{
					struct CLOSURE : IShaderViewGrabber
					{
						bool success = false;
						void OnGrab(const ShaderView& s)
						{
							if SUCCEEDED(s.hr)
							{
								success = true;
							}
						}
					} g;
					pShaders->GrabShaderObject(bad.id, g);
					return g.success;
				}
			);

			if (i != badShaders.end())
			{
				updateCount++;
			}

			badShaders.erase(i, badShaders.end());

			return updateCount > 0;
		}

		HRESULT lastBadHR = S_OK;

		void UpdateFrame() override
		{
			if (UpdateShaderState())
			{
				ShowShaderErrors(this->hReportWnd);
				return;
			}
			
			if (badShaders.empty())
			{
				wc.evHandler.OnUpdateFrame(idBackBuffer);
				if (swapChain)
				{
					HRESULT hr = swapChain->Present(1, 0);
					if FAILED(hr)
					{
						lastBadHR = hr;
					}
				}
			}
		}
	};
}

namespace Rococo::Graphics
{
	IDX11Window* CreateDX11Window(IDX11System& system, DX11WindowContext& wc, ITextureSupervisor& textures)
	{
		auto* w = new ANON::DX11Window(system, wc, textures);
		w->ActivateCustomWindowsProcedureAndMakeVisible();
		w->WaitFrames(10, 10);
		return w;
	}
}