#include "sexystudio.impl.h"
#include "resource.h"
#include <rococo.maths.h>

using namespace Rococo;

namespace Rococo::SexyStudio
{
	const char* GetChildClassName()
	{
		return  "SexyStudioChild";
	}

	void PopulatChildClass(HINSTANCE hInstance, WNDCLASSEXA& classDef)
	{
		classDef = { 0 };
		classDef.cbSize = sizeof(classDef);
		classDef.style = 0;
		classDef.cbWndExtra = 0;
		classDef.hbrBackground = (HBRUSH)COLOR_WINDOW;
		classDef.hCursor = LoadCursor(nullptr, IDC_ARROW);
		classDef.hIcon = nullptr;
		classDef.hIconSm = nullptr;
		classDef.hInstance = hInstance;
		classDef.lpszClassName = GetChildClassName();
		classDef.lpszMenuName = NULL;
		classDef.lpfnWndProc = DefWindowProcA;
	}

	static ATOM childClassAtom = 0;

	struct Win32WindowContext
	{
		HDC hMemDC = nullptr;
		HBITMAP hBitmap = nullptr;
		HBITMAP hOldBitmap = nullptr;

		~Win32WindowContext()
		{
			if (hMemDC != nullptr)
			{
				SelectObject(hMemDC, (HGDIOBJ)hOldBitmap);
				if (hBitmap)
				{
					DeleteObject((HGDIOBJ)hBitmap);
				}
				DeleteDC(hMemDC);
			}
		}
	};

	Win32ChildWindow::Win32ChildWindow(HWND hParent, IWin32WindowMessageLoopHandler& _handler):
		handler(_handler), windowContext(nullptr)
	{
		if (childClassAtom == 0)
		{
			WNDCLASSEXA childInfo;
			PopulatChildClass(GetMainInstance(), childInfo);
			childClassAtom = RegisterClassExA(&childInfo);
			if (childClassAtom == 0)
			{
				Throw(GetLastError(), "Could not create child window class atom");
			}
		}

		DWORD exStyle = 0;
		DWORD style = WS_CHILD;
		HMENU hMenu = nullptr;
	
		hWnd = CreateWindowExA(
			exStyle,
			"SexyStudioChild",
			"",
			style,
			0, // x
			0, // y
			0, // width
			0, // height
			hParent,
			hMenu,
			GetMainInstance(),
			nullptr);

		if (hWnd == nullptr)
		{
			Throw(0, "%s: could not create child window", __FUNCTION__);
		}

		struct ANON
		{
			static LRESULT WINAPI RouteMessage(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
			{
				auto* handler = (IWindowMessageHandler*)GetWindowLongPtrA(hWnd, GWLP_USERDATA);

				try
				{
					return handler->ProcessMessage(msg, wParam, lParam);
				}
				catch (IException& ex)
				{
					Rococo::SexyStudio::SetLastMessageError(ex);
					return DefWindowProc(hWnd, msg, wParam, lParam);
				}
			}

		};

		SetWindowLongPtrA(hWnd, GWLP_USERDATA, (LONG_PTR)&handler);
		SetWindowLongPtrA(hWnd, GWLP_WNDPROC, (LONG_PTR) ANON::RouteMessage);

		windowContext = new Win32WindowContext();
	}

	Win32ChildWindow::~Win32ChildWindow()
	{
		DestroyWindow(hWnd);

		delete windowContext;
	}

	BufferedPaintStruct::BufferedPaintStruct(Win32ChildWindow& _window) :
		window(_window)
	{
		HDC dc = BeginPaint(window, this);
		if (dc == nullptr)
		{
			memDC = nullptr;
			return;
		}

		auto& context = *window.windowContext;

		if (context.hMemDC == nullptr)
		{
			RECT rect;
			GetClientRect(window, &rect);
			context.hMemDC = memDC = CreateCompatibleDC(dc);
			context.hBitmap = CreateCompatibleBitmap(dc, rect.right, rect.bottom);
			if (context.hBitmap == nullptr)
			{
				DeleteDC(context.hMemDC);
				context.hMemDC = nullptr;
				Throw(GetLastError(), "%s CreateCompatibleBitmap failed.", __FUNCTION__);
			}
		}
		else
		{
			memDC = context.hMemDC;
		}

		SetMapMode(memDC, MM_TEXT);

		// Target the context bitmap for writing
		context.hOldBitmap = (HBITMAP)SelectObject(memDC, (HGDIOBJ)context.hBitmap);
	}

	void BufferedPaintStruct::RenderToScreen()
	{
		if (memDC)
		{
			auto& context = *window.windowContext;

			RECT rect;
			GetClientRect(window, &rect);

			BitBlt(hdc, 0, 0, rect.right, rect.bottom, memDC, 0, 0, SRCCOPY);

			SelectObject(memDC, context.hOldBitmap);
		}
	}

	BufferedPaintStruct::~BufferedPaintStruct()
	{
		EndPaint(window, this);
	}

	namespace Widgets
	{
		GuiRect GetRect(IGuiWidget& widget)
		{
			RECT rect;
			GetWindowRect(widget.Window(), &rect);
			return GuiRect{ rect.left, rect.top, rect.right, rect.bottom };
		}

		Vec2i GetSpan(IGuiWidget& widget)
		{
			RECT rect;
			GetClientRect(widget.Window(), &rect);
			return { rect.right - rect.left, rect.bottom - rect.top };
		}

		Vec2i GetParentSpan(IGuiWidget& widget)
		{
			RECT rect;
			GetClientRect(GetParent(widget.Window()), &rect);
			return { rect.right - rect.left, rect.bottom - rect.top };
		}

		void SetText(IWindow& window, const char* text)
		{
			SetWindowTextA(window, text);
		}

		void SetWidgetPosition(IGuiWidget& widget, const GuiRect& rect)
		{
			MoveWindow(widget.Window(), rect.left, rect.top, Width(rect), Height(rect), TRUE);
		}

		void Maximize(IWindow& window)
		{
			ShowWindow(window, SW_MAXIMIZE);
		}

		void Minimize(IWindow& window)
		{
			ShowWindow(window, SW_MINIMIZE);
		}
	} // Widgets
} // Roococ::SexyStudio