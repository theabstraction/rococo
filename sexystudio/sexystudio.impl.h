#pragma once

#include "sexystudio.api.h" // The OS independent part of the sexystudio API
#include <rococo.os.win32.h>
#include <rococo.window.h>

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

	class Brush
	{
	public:
		HBRUSH hBrush = nullptr;

		operator HBRUSH ()
		{
			return hBrush;
		}

		~Brush()
		{
			if (!hBrush) DeleteObject(hBrush);
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

	struct BufferedPaintStruct : public PAINTSTRUCT
	{
	public:
		BufferedPaintStruct(Win32ChildWindow& window);
		~BufferedPaintStruct();

		void RenderToScreen();
		HDC GetMemDC() { return memDC; }
	private:
		Win32ChildWindow& window;
		HDC memDC;
	};

	struct Win32WindowContext;

	class Win32ChildWindow: public Rococo::Windows::IWindow
	{
		friend struct BufferedPaintStruct;
	private:
		HWND hWnd;
		IWin32WindowMessageLoopHandler& handler;
		Win32WindowContext* windowContext;
	public:
		Win32ChildWindow(HWND hParent, IWin32WindowMessageLoopHandler& handler);
		~Win32ChildWindow();
		operator HWND() const override {return hWnd; }
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

	// used as the template paramter for Rococo::Events::TEventArgs<T>;
}
