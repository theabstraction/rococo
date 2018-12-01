#include <rococo.types.h>
#include <rococo.os.win32.h>
#include <rococo.window.h>
#include <rococo.cute.h>

using namespace Rococo;
using namespace Rococo::Cute;

struct SplitterRibbon : IChildSupervisor
{
	HWND hParentWnd;
	HWND hChildWnd;

	void Close() override
	{

	}

	void OnPaint()
	{
		PAINTSTRUCT ps;
		HDC dc = BeginPaint(hChildWnd, &ps);

		RECT rect;
		GetClientRect(hChildWnd, &rect);

		HBRUSH hBrush = CreateSolidBrush(RGB(192, 192, 192));
		FillRect(dc, &rect, hBrush);
		DeleteObject(hBrush);

		DrawEdge(dc, &rect, EDGE_BUMP, BF_RECT);

		EndPaint(hChildWnd, &ps);
	}

	static LRESULT OnSplitterRibbonMessage(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
	{
		auto* This = (SplitterRibbon*) GetWindowLongPtrA(hWnd, GWLP_USERDATA);

		switch (uMsg)
		{
		case WM_PAINT:
			This->OnPaint();
			return 0;
		case WM_ERASEBKGND:
			return 0;
		default:
			break;
		}
		return DefWindowProcA(hWnd, uMsg, wParam, lParam);
	}

	SplitterRibbon(HWND _hParentWnd, DWORD style, DWORD exStyle, int32 x, int32 y, int32 dx, int32 dy) : hParentWnd(_hParentWnd)
	{
		auto hInstance = (HINSTANCE)GetWindowLongPtrA(hParentWnd, GWLP_HINSTANCE);

		WINDOWINFO info;
		info.cbSize = sizeof(info);
		if (!GetWindowInfo(hParentWnd, &info))
		{
			Throw(GetLastError(), "CuteChild::GetWindowInfo(...) failed");
		}

		cstr wc = (cstr)info.atomWindowType;
		hChildWnd = CreateWindowExA(exStyle, wc, "", style, x, y, dx, dy, hParentWnd, NULL, hInstance, NULL);
		if (hChildWnd == NULL)
		{
			Throw(GetLastError(), "CuteChild::CreateWindowExA(...) failed");
		}

		SetWindowLongPtrA(hChildWnd, GWLP_USERDATA, (LONG_PTR)this);
		SetWindowLongPtrA(hChildWnd, GWLP_WNDPROC, (LONG_PTR) OnSplitterRibbonMessage);
	}

	void Free() override
	{
		delete this;
	}

	WindowRef Handle() override
	{
		return ToRef(hChildWnd);
	}

	void OnResize(Vec2i span, ResizeType type)
	{

	}
};

struct Splitter : public ISplitSupervisor, virtual ISplit, virtual IWindowSupervisor
{
	HWND hParentWnd;
	HWND hContainerWnd;
	int32 pixelSplit;
	int32 splitterWidth;
	boolean32 draggable;
	bool isLeftAndRight;

	AutoFree<IChildSupervisor> splitterWnd;
	AutoFree<IChildSupervisor> hi; 
	AutoFree<IChildSupervisor> lo; 

	Splitter(ATOM atom, HWND hParentWnd, int32 pixelSplit, int32 splitterWidth, boolean32 draggable, bool isLeftAndRight)
	{
		this->hParentWnd = hParentWnd;
		this->pixelSplit = pixelSplit;
		this->splitterWidth = splitterWidth;
		this->isLeftAndRight = isLeftAndRight;

		RECT rect;
		GetClientRect(hParentWnd, &rect);
		int32 dx = rect.right;
		int32 dy = rect.bottom;

		DWORD exStyle = 0;
		DWORD style = WS_CHILD | WS_VISIBLE;

		auto hInstance = (HINSTANCE)GetWindowLongPtrA(hParentWnd, GWLP_HINSTANCE);

		char name[256];
		if (0 >= GetClassNameA(hParentWnd, name, sizeof(name)))
		{
			Throw(GetLastError(), "CuteChild::GetClassNameA(_hParentWnd) failed");
		}

		hContainerWnd = CreateWindowExA(exStyle, name, "", style, 0, 0, dx, dy, hParentWnd, NULL, hInstance, NULL);
		if (hContainerWnd == NULL)
		{
			Throw(GetLastError(), "hContainerWnd = CreateWindowExA(...) failed");
		}

		Native::SetColourTarget(ToRef(hParentWnd), ColourTarget_NormalBackground, RGBAb(0, 0, 0, 0));
		Native::SetColourTarget(ToRef(hContainerWnd), ColourTarget_NormalBackground, RGBAb(0, 0, 0, 0));
		
		int32 hiX, hiY, loX, loY;
		int32 hiDX, hiDY, loDX, loDY;
		int32 spX, spY, spDX, spDY;

		if (isLeftAndRight)
		{
			loX = 0;
			loY = 0;
			loDX = pixelSplit;
			loDY = dy;

			spX = loDX;
			spY = 0;
			spDX = splitterWidth;
			spDY = dy;
			
			hiX = loDX + splitterWidth;
			hiY = 0;
			hiDX = dx - hiX;
			hiDY = dy;
		}
		else
		{
			hiX = 0;
			hiY = 0;
			hiDX = dx;
			hiDY = pixelSplit;

			spX = 0;
			spY = hiDY;
			spDX = dx;
			spDY = splitterWidth;

			loX = 0;
			loY = 0;
			loDX = dx;
			loDY = dy - hiDY - splitterWidth;
		}

		hi = CreateChild(hContainerWnd, style, exStyle, hiX, hiY, hiDX, hiDY);
		lo = CreateChild(hContainerWnd, style, exStyle, loX, loY, loDX, loDY);
		splitterWnd = new SplitterRibbon(hContainerWnd, style, exStyle, spX, spY, spDX, spDY);
	}

	void Close() override
	{

	}

	void Free() override
	{
		delete this;
	}

	ISplit& Split() override
	{
		return *this;
	}

	IWindowBase* Lo() override
	{
		return lo;
	}

	IWindowBase* Hi() override
	{
		return hi;
	}

	WindowRef Handle() override
	{
		return ToRef(hContainerWnd);
	}

	void OnResize(Vec2i span, ResizeType type)
	{
		int32 hiX, hiY, loX, loY;
		int32 hiDX, hiDY, loDX, loDY;
		int32 spX, spY, spDX, spDY;

		if (isLeftAndRight)
		{
			loX = 0;
			loY = 0;
			loDX = pixelSplit;
			loDY = span.y;

			spX = loDX;
			spY = 0;
			spDX = splitterWidth;
			spDY = span.y;

			hiX = loDX + splitterWidth;
			hiY = 0;
			hiDX = span.x - hiX;
			hiDY = span.y;
		}
		else
		{
			hiX = 0;
			hiY = 0;
			hiDX = span.x;
			hiDY = pixelSplit;

			spX = 0;
			spY = hiDY;
			spDX = span.x;
			spDY = splitterWidth;

			loX = 0;
			loY = 0;
			loDX = span.x;
			loDY = span.y - hiDY - splitterWidth;
		}

		MoveWindow(hContainerWnd, 0, 0, span.x, span.y, FALSE);
		MoveWindow(ToHWND(hi->Handle()), hiX, hiY, hiDX, hiDY, FALSE);
		MoveWindow(ToHWND(lo->Handle()), loX, loY, loDX, loDY, FALSE);
		MoveWindow(ToHWND(splitterWnd->Handle()), spX, spY, spDX, spDY, FALSE);

		InvalidateRect(ToHWND(hi->Handle()), nullptr, TRUE);
		InvalidateRect(ToHWND(lo->Handle()), nullptr, TRUE);
		InvalidateRect(ToHWND(splitterWnd->Handle()), nullptr, TRUE);
	}
};

namespace Rococo
{
	namespace Cute
	{
		ISplitSupervisor* CreateSplit(ATOM atom, HWND hParentWnd, int32 pixelSplit, int32 splitterWidth, boolean32 draggable, bool isLeftAndRight)
		{
			auto* s = new Splitter(atom, hParentWnd, pixelSplit, splitterWidth, draggable, isLeftAndRight);
			SetMasterProc(s->Handle(), s);
			return s;
		}
	}
}