#pragma once

#include "sexystudio.api.h" // The OS independent part of the sexystudio API
#include <rococo.os.win32.h>
#include <rococo.window.h>
#include <Uxtheme.h>
#include <windowsx.h>

namespace Rococo
{
	namespace Events
	{
		class IPublisher;
		struct EventIdRef;
	}
}

namespace Rococo::SexyStudio
{
	using namespace Rococo;
	using namespace Rococo::Windows;

	const char* GetChildClassName();

	struct ILayout;
	struct IWin32WindowMessageLoopHandler;
	struct IWindowMessageHandler;
	struct IToolbar;
	struct ILayoutSet;
	struct IWidgetSet;
	struct IGuiWidget;

	struct IOSFont
	{
		virtual operator HFONT () = 0;
	};

	class Font : public IOSFont
	{
		HFONT hFont;
	public:
		operator HFONT () override
		{
			return hFont;
		}

		Font(LOGFONTA logFont);
		~Font();
	};

	class Brush
	{
	private:
		HBRUSH hBrush = nullptr;

	public:
		operator HBRUSH() { return hBrush; }

		Brush& operator = (COLORREF color)
		{
			if (hBrush)
			{
				DeleteObject(hBrush);
			}

			hBrush = CreateSolidBrush(color);
			return *this;
		}

		~Brush()
		{
			if (!hBrush) DeleteObject(hBrush);
		}
	};

	class Pen
	{
	private:
		HPEN hPen = nullptr;

	public:
		operator HPEN() { return hPen; }

		Pen& operator = (COLORREF color)
		{
			if (hPen)
			{
				DeleteObject(hPen);
			}

			hPen = CreatePen(PS_SOLID, 1, color);
			return *this;
		}

		~Pen()
		{
			if (!hPen) DeleteObject(hPen);
		}
	};

	ROCOCOAPI IWin32WindowMessageLoopHandler
	{
		virtual LRESULT ProcessMessage(UINT msg, WPARAM wParam, LPARAM lParam) = 0;
	};

	ROCOCOAPI IWindowMessageHandler: IWin32WindowMessageLoopHandler
	{
		virtual void Show() = 0;
		virtual void Free() = 0;
	};

	class Win32ChildWindow;
	class Win32PopupWindow;

	struct Win32WindowContext;

	class Win32ChildWindow: public Rococo::Windows::IWindow
	{
	private:
		HWND hWnd;
		IWin32WindowMessageLoopHandler& handler;
	public:
		Win32ChildWindow(HWND hParent, IWin32WindowMessageLoopHandler& handler, DWORD extraStyleFlags = 0);
		~Win32ChildWindow();
		operator HWND() const override {return hWnd; }
	};

	class Win32PopupWindow : public Rococo::Windows::IWindow
	{
	private:
		HWND hWnd;
		IWin32WindowMessageLoopHandler& handler;
	public:
		Win32PopupWindow(HWND hParent, IWin32WindowMessageLoopHandler& handler, DWORD extraStyleFlags = 0);
		~Win32PopupWindow();
		operator HWND() const override { return hWnd; }
	};

	class Win32TopLevelWindow : public Rococo::Windows::IWindow
	{
	private:
		HWND hWnd;
		IWin32WindowMessageLoopHandler& handler;
	public:
		Win32TopLevelWindow(DWORD exStyle, DWORD style, IWin32WindowMessageLoopHandler& handler);
		~Win32TopLevelWindow();
		operator HWND() const override { return hWnd; }
	};

	void SetLastMessageError(IException& ex);
	void AssertNoMessageError();

	HINSTANCE GetMainInstance();

	void SetPosition(IWindow& window, const GuiRect& rect);
	Vec2i GetSpan(IWindow& window);

	void InitStudioWindows(HINSTANCE hInstance, cstr iconLarge, cstr iconSmall);

	IToolbar* CreateToolbar(IWidgetSet& widgets);

	enum { WM_IDE_RESIZED = 0x8001 };

	struct HWNDProxy: Rococo::Windows::IWindow
	{
		HWND hWnd;
		HWNDProxy(HWND _hWnd) : hWnd(_hWnd) {}
		operator HWND() const override { return hWnd;  }
	};

	ROCOCOAPI IWin32Painter
	{
		virtual void OnPaint(HDC dc) = 0;
	};

	void PaintDoubleBuffered(HWND hWnd, IWin32Painter& paint);

	// used as the template paramter for Rococo::Events::TEventArgs<T>;

	COLORREF ToCOLORREF(RGBAb b);
}
