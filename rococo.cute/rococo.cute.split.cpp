#include <rococo.types.h>
#include <rococo.os.win32.h>
#include <rococo.window.h>
#include "rococo.cute.h"

#include "cute.sxh.h"

using namespace Rococo;
using namespace Rococo::Cute;

struct SplitterRibbon : IChildSupervisor, IWindowBase
{
	HWND hParentWnd;
	HWND hChildWnd;

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

	size_t GetWindowHandle() override
	{
		return (size_t)hChildWnd;
	}

	IWindowBase* Window() override
	{
		return this;
	}
};

struct Splitter : public ISplitSupervisor, public ISplit
{
	HWND hParentWnd;
	int32 pixelSplit;
	int32 splitterWidth;
	boolean32 draggable;
	bool isLeftAndRight;

	AutoFree<IChildSupervisor> containerWnd;
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
		containerWnd = CreateChild(hParentWnd, style, exStyle, 0, 0, dx, dy);
		
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

		hi = CreateChild(*containerWnd->Window(), style, exStyle, hiX, hiY, hiDX, hiDY);
		lo = CreateChild(*containerWnd->Window(), style, exStyle, loX, loY, loDX, loDY);
		splitterWnd = new SplitterRibbon((HWND)containerWnd->Window()->GetWindowHandle(), style, exStyle, spX, spY, spDX, spDY);
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
		return hi->Window();
	}

	IWindowBase* Hi() override
	{
		return lo->Window();
	}

	size_t GetWindowHandle() override
	{
		return (size_t)containerWnd->Window()->GetWindowHandle();
	}
};

namespace Rococo
{
	namespace Cute
	{
		ISplitSupervisor* CreateSplit(ATOM atom, HWND hParentWnd, int32 pixelSplit, int32 splitterWidth, boolean32 draggable, bool isLeftAndRight)
		{
			auto* s = new Splitter(atom, hParentWnd, pixelSplit, splitterWidth, draggable, isLeftAndRight);
			return s;
		}
	}
}