#include <rococo.os.win32.h>
#include <rococo.strings.h>
#include <rococo.api.h>
#include <rococo.window.h>
#include <rococo.maths.h>
#include <rococo.dx12.h>
#include "rococo.dx12.helpers.inl"
#include <rococo.auto-release.h>

#include <d3d12.h>
#include <dxgi1_6.h>

#include "rococo.dx12-msoft.h" // If this does not compile update your Windows 10 kit thing

#include <vector>

using namespace Rococo;
using namespace Rococo::Graphics;

namespace ANON
{
	ATOM dx12Atom = 0;
	char atomName[64];

	void RegisterWndHandler(HINSTANCE hResourceInstance)
	{
		if (dx12Atom != 0) return;

		enum { IDI_ICON1 = 101 };

		WNDCLASSEXA classDef = { 0 };
		classDef.cbSize = sizeof(classDef);
		classDef.style = 0;
		classDef.cbWndExtra = 0;
		classDef.hbrBackground = (HBRUSH)COLOR_WINDOW;
		classDef.hCursor = LoadCursor(nullptr, IDC_ARROW);
		classDef.hIcon = LoadIconA(hResourceInstance, (const char* ) IDI_ICON1);
		classDef.hIconSm = LoadIconA(hResourceInstance, (const char*) IDI_ICON1);
		classDef.hInstance = hResourceInstance;

		SafeFormat(atomName, "Rococo-DX12-%llx", Rococo::Time::TickCount());
		classDef.lpszClassName = atomName;
		classDef.lpszMenuName = NULL;
		classDef.lpfnWndProc = DefWindowProcA;

		dx12Atom = RegisterClassExA(&classDef);

		if (dx12Atom == 0)
		{
			Throw(GetLastError(), "Error creating DX12 Window class atom");
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

	HWND CreateWindowFromContext(DX12WindowCreateContext& context)
	{
		DWORD exStyle;
		DWORD style;

		if (!context.hParentWnd)
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
			(const char*)dx12Atom,
			context.title,
			style,
			context.rect.left,
			context.rect.top,
			Width(context.rect),
			Height(context.rect),
			(HWND) context.hParentWnd,
			nullptr,
			(HINSTANCE) context.hResourceInstance,
			nullptr
		);

		return hMainWnd;
	}

	class DX12RendererWindow : public IDX12RendererWindow, public Rococo::Windows::IWindow
	{
	private:
		DX12WindowInternalContext ic;
		IDX12RendererWindowEventHandler& evHandler; // The DX12 consumer's event handler
		HWND hMainWnd = nullptr;
		bool hasFocus = false;
		HKL keyboardLayout = nullptr;
		Vec2i minSpan{ 800, 600 };
		uint32 backBufferIndex = 0;
		AutoRelease<IDXGISwapChain4> swapChain;
		AutoRelease<ID3D12DescriptorHeap> rtvHeap;
		uint32 rtvDescriptorSize = 0;
		std::vector<AutoRelease<ID3D12Resource>> renderTargets;
		AutoRelease<ID3D12CommandAllocator> commandAllocator;
		uint32 frameIndex = 0;
		HFONT consoleFont = NULL;

		virtual ~DX12RendererWindow()
		{
			if (hMainWnd) DestroyWindow(hMainWnd);

			if (consoleFont) DeleteObject((HGDIOBJ)consoleFont);
		}

		void RegisterRawInput()
		{

		}

		HString msg;

		void WaitForNextRenderAndDisplay(cstr message)
		{
			msg = message;

			uint64 releaseAt = frameCount + 10;

			while (releaseAt > frameCount)
			{
				MsgWaitForMultipleObjects(0, NULL, FALSE, 10, QS_ALLINPUT);

				InvalidateRect(hMainWnd, NULL, TRUE);

				MSG msg;
				while (PeekMessageA(&msg, nullptr, 0, 0, PM_REMOVE))
				{
					TranslateMessage(&msg);
					DispatchMessageA(&msg);
				}
			}
		}

		void SetText(cstr text)
		{
			if (!Eq(msg, text))
			{
				msg = text;
				InvalidateRect(hMainWnd, NULL, TRUE);
			}
		}

		void OnActivate()
		{
			ic.factory.MakeWindowAssociation(hMainWnd, 0);
		}

		void OnDeactivate()
		{
			ic.factory.MakeWindowAssociation(nullptr, 0);
		}

		void OnSize(Vec2i newSpan)
		{

		}

		LRESULT RouteInput(WPARAM wParam, LPARAM lParam)
		{
			return 0;
		}

		uint64 frameCount = 0;

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
				RegisterRawInput();
				return 0L;
			case WM_KILLFOCUS:
				hasFocus = false;
				return 0L;
			case WM_GETMINMAXINFO:
				{
					auto* m = (MINMAXINFO*)lParam;
					m->ptMinTrackSize = POINT{ minSpan.x, minSpan.y };
				}
			return 0L;
			case WM_INPUTLANGCHANGE:
				keyboardLayout = GetKeyboardLayout(0);
				return 0L;
			case WM_SIZE:
				OnSize(Vec2i{ LOWORD(lParam), HIWORD(lParam) });
				return 0L;
			case WM_INPUT:
				return RouteInput(wParam, lParam);
			case WM_CLOSE:
				evHandler.OnCloseRequested(*this);
				return 0L;
			case WM_ERASEBKGND:
				return 0L;
			case WM_PAINT:
				{
					PAINTSTRUCT ps;
					HDC dc = BeginPaint(hMainWnd, &ps);
					FillRect(dc, &ps.rcPaint, (HBRUSH)(COLOR_WINDOW + 1));

					auto old = SelectObject(dc, (HGDIOBJ)consoleFont);

					RECT rect;
					GetClientRect(hMainWnd, &rect);
					DrawTextA(dc, msg, -1, &rect, DT_VCENTER | DT_LEFT);
					frameCount++;

					SelectObject(dc, old);

					EndPaint(hMainWnd, &ps);
					return 0L;
				}
			}

			return DefWindowProcA(hMainWnd, uMsg, wParam, lParam);
		}

		static LRESULT OnMessage(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
		{
			auto* This = (DX12RendererWindow*)GetWindowLongPtrA(hWnd, GWLP_USERDATA);

			try
			{
				return This->OnMessageProtected(uMsg, wParam, lParam);
			}
			catch (IException& ex)
			{
				This->evHandler.OnMessageQueueException(*This, ex);
				return DefWindowProcA(hWnd, uMsg, wParam, lParam);
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
	public:
		DX12RendererWindow(DX12WindowInternalContext& ref_ic, DX12WindowCreateContext& context) :
			ic(ref_ic), evHandler(context.evHandler)
		{
			RegisterWndHandler((HINSTANCE) context.hResourceInstance);
			hMainWnd = CreateWindowFromContext(context);
			keyboardLayout = GetKeyboardLayout(0);

			try
			{
				DXGI_SWAP_CHAIN_DESC1 winDesc;
				GetSwapChainForWindow(hMainWnd, winDesc);
				DXGI_SWAP_CHAIN_FULLSCREEN_DESC fullDesc;
				GetSwapChainForFullscreen(hMainWnd, fullDesc);

				AutoRelease<IDXGISwapChain1> swapChain1;
				VALIDATE_HR(ic.factory.CreateSwapChainForHwnd(&ic.q, hMainWnd, &winDesc, &fullDesc, NULL, &swapChain1));
				VALIDATE_HR(swapChain1->QueryInterface(&swapChain));

				backBufferIndex = swapChain->GetCurrentBackBufferIndex();

				D3D12_DESCRIPTOR_HEAP_DESC rtvHeapDesc = {};
				rtvHeapDesc.NumDescriptors = winDesc.BufferCount;
				rtvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
				rtvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
				VALIDATE_HR(ic.device.CreateDescriptorHeap(&rtvHeapDesc, IID_PPV_ARGS(&rtvHeap)));

				rtvDescriptorSize = ic.device.GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);

				CD3DX12_CPU_DESCRIPTOR_HANDLE rtvHandle(rtvHeap->GetCPUDescriptorHandleForHeapStart());

				renderTargets.resize(2);

				// Create a RTV for each frame.
				for (UINT i = 0; i < winDesc.BufferCount; i++)
				{
					VALIDATE_HR(swapChain->GetBuffer(i, IID_PPV_ARGS(&renderTargets[i])));
					ic.device.CreateRenderTargetView(renderTargets[i], nullptr, rtvHandle);
					rtvHandle.Offset(1, rtvDescriptorSize);
				}

				LOGFONTA logFont = { 0 };
				SafeFormat(logFont.lfFaceName, "Consolas");
				logFont.lfHeight = -11;

				consoleFont = CreateFontIndirectA(&logFont);
			}
			catch (IException&)
			{
				DestroyWindow(hMainWnd);
				throw;
			}
		}

		void Render()
		{
		}

		void ActivateCustomWindowsProcedureAndMakeVisible()
		{
			SetWindowLongPtrA(hMainWnd, GWLP_USERDATA, (LONG_PTR)this);
			SetWindowLongPtrA(hMainWnd, GWLP_WNDPROC, (LONG_PTR) OnMessage);
			ShowWindow(hMainWnd, SW_SHOW);
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
	};
}

namespace Rococo::DX12Impl
{
	IDX12RendererWindow* CreateDX12Window(DX12WindowInternalContext& ic, DX12WindowCreateContext& context)
	{
		auto* window = new ANON::DX12RendererWindow(ic, context);
		window->ActivateCustomWindowsProcedureAndMakeVisible();
		return window;
	}
}