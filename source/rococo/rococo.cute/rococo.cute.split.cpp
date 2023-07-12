#include <rococo.types.h>
#include <rococo.os.win32.h>
#include <rococo.window.h>
#include <rococo.cute.h>

using namespace Rococo;
using namespace Rococo::Cute;

struct Splitter;
IChildSupervisor* CreateSplitterRibbon(Splitter& splitter, DWORD style, DWORD exStyle, int32 x, int32 y, int32 dx, int32 dy);

struct Splitter : public ISplitSupervisor, virtual ISplit
{
	IParentWindow& parent;
	HWND hContainerWnd;
	int32 pixelSplit;
	int32 splitterWidth;
	boolean32 draggable;
	bool isLeftAndRight;
	int32 minLo;
	int32 maxHi;
	Post::IPostbox& postbox;

	AutoFree<IChildSupervisor> splitterWnd;
	AutoFree<IParentWindow> hi;
	AutoFree<IParentWindow> lo;

	Splitter(ATOM atom, IParentWindow& _parent, int32 pixelSplit, int32 minLo, int32 maxHi, 
		int32 splitterWidth, boolean32 draggable, bool isLeftAndRight, Post::IPostbox& post):
		parent(_parent),
		postbox(post)
	{
		this->pixelSplit = pixelSplit;
		this->splitterWidth = splitterWidth;
		this->isLeftAndRight = isLeftAndRight;
		this->minLo = minLo;
		this->maxHi = maxHi;

		RECT rect;
		GetClientRect(ToHWND(parent.Handle()), &rect);
		int32 dx = rect.right;
		int32 dy = rect.bottom;

		DWORD exStyle = 0;
		DWORD style = WS_CHILD | WS_VISIBLE;

		auto hInstance = (HINSTANCE)GetWindowLongPtrA(ToHWND(parent.Handle()), GWLP_HINSTANCE);

		char name[256];
		if (0 >= GetClassNameA(ToHWND(parent.Handle()), name, sizeof(name)))
		{
			Throw(GetLastError(), "CuteChild::GetClassNameA(_hParentWnd) failed");
		}

		hContainerWnd = CreateWindowExA(exStyle, name, "", style, 0, 0, dx, dy, ToHWND(parent.Handle()), NULL, hInstance, NULL);
		if (hContainerWnd == NULL)
		{
			Throw(GetLastError(), "hContainerWnd = CreateWindowExA(...) failed");
		}

		Native::SetColourTarget(parent.Handle(), ColourTarget_NormalBackground, RGBAb(0, 0, 0, 0));
		Native::SetColourTarget(ToRef(hContainerWnd), ColourTarget_NormalBackground, RGBAb(0, 0, 0, 0));

		hi = CreateParent(*this, style, exStyle, 0, 0, 3, 4, postbox);
		lo = CreateParent(*this, style, exStyle, 3, 0, 3, 4, postbox);
		splitterWnd = CreateSplitterRibbon(*this, style, exStyle, 4, 0, 2, 4);
		
		Layout();
	}

	void AddChild(IWindowSupervisor* child) override
	{
		
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

	IParentWindow* Lo() override
	{
		return lo;
	}

	IParentWindow* Hi() override
	{
		return hi;
	}

	WindowRef Handle() override
	{
		return ToRef(hContainerWnd);
	}

	void Layout()
	{
		RECT rect;
		GetClientRect(hContainerWnd, &rect);
		Vec2i span = { rect.right, rect.bottom };

		pixelSplit = max(pixelSplit, minLo);
		pixelSplit = min(pixelSplit, (int)rect.right - maxHi);

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

		MoveWindow(ToHWND(hi->Handle()), hiX, hiY, hiDX, hiDY, FALSE);
		MoveWindow(ToHWND(lo->Handle()), loX, loY, loDX, loDY, FALSE);
		MoveWindow(ToHWND(splitterWnd->Handle()), spX, spY, spDX, spDY, FALSE);

		InvalidateRect(ToHWND(hi->Handle()), nullptr, TRUE);
		InvalidateRect(ToHWND(lo->Handle()), nullptr, TRUE);
		InvalidateRect(ToHWND(splitterWnd->Handle()), nullptr, TRUE);
	}

	void OnResize(Vec2i span, ResizeType type)
	{
		Layout();
	}


	void RepaintSides(bool repaintLo, bool repaintHi)
	{
		if (repaintLo)
		{
			auto hLo = ToHWND(lo->Handle());
			InvalidateRect(hLo, nullptr, TRUE);
			UpdateWindow(hLo);
		}

		if (repaintHi)
		{
			auto hHi = ToHWND(hi->Handle());
			InvalidateRect(hHi, nullptr, TRUE);
			UpdateWindow(hHi);
		}
	}
};

struct SplitterRibbon : IChildSupervisor
{
	Splitter& splitter;
	HWND hChildWnd;

	bool isDragging = false;
	POINT dragFrom;

	void AddChild(IWindowSupervisor* child)
	{
		Throw(0, "SplitterRibbon does not take children");
	}

	void Close() override
	{

	}

	void RenderDragBar()
	{
		POINT p;
		GetCursorPos(&p);
		ScreenToClient(splitter.hContainerWnd, &p);

		HDC dc = GetDC(splitter.hContainerWnd);

		POINT delta = { p.x - dragFrom.x, p.y - dragFrom.y };

		if (splitter.isLeftAndRight)
		{
			RECT rect;
			GetWindowRect(hChildWnd, &rect);
			rect.left += delta.x;
			rect.right += delta.x;
			
			POINT topLeft = { rect.left, rect.top };
			POINT bottomRight = { rect.right, rect.bottom };
			ScreenToClient(splitter.hContainerWnd, &topLeft);
			ScreenToClient(splitter.hContainerWnd, &bottomRight);

			splitter.RepaintSides(delta.x < 0, delta.x > 0);

			RECT renderRect = { topLeft.x, topLeft.y, bottomRight.x, bottomRight.y };
			DrawEdge(dc, &renderRect, EDGE_ETCHED, BF_LEFT | BF_RIGHT);
		}

		ReleaseDC(splitter.hContainerWnd, dc);
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

		DrawEdge(dc, &rect, EDGE_RAISED, BF_LEFT | BF_RIGHT);

		EndPaint(hChildWnd, &ps);
	}

	void BeginDrag()
	{
		GetCursorPos(&dragFrom);
		ScreenToClient(splitter.hContainerWnd, &dragFrom);

		isDragging = true;
		SetCapture(hChildWnd);
	}

	void EndDrag()
	{
		isDragging = false;
		SetCapture(nullptr);

		POINT p;
		GetCursorPos(&p);
		ScreenToClient(splitter.hContainerWnd, &p);

		POINT delta = { p.x - dragFrom.x, p.y - dragFrom.y };

		if (splitter.isLeftAndRight)
		{
			splitter.pixelSplit += delta.x;
		}

		splitter.Layout();
		splitter.RepaintSides(true, true);
	}

	static LRESULT OnSplitterRibbonMessage(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
	{
		auto* This = (SplitterRibbon*)GetWindowLongPtrA(hWnd, GWLP_USERDATA);

		switch (uMsg)
		{
		case WM_PAINT:
			This->OnPaint();
			return 0;
		case WM_ERASEBKGND:
			return 0;
		case WM_MOUSEMOVE:
			if (This->isDragging)
			{
				This->RenderDragBar();
			}
			return 0;
		case WM_LBUTTONDOWN:
			This->BeginDrag();
			return 0L;
		case WM_LBUTTONUP:
			This->EndDrag();
			return 0L;
		default:
			break;
		}
		return DefWindowProcA(hWnd, uMsg, wParam, lParam);
	}

	SplitterRibbon(Splitter& _splitter, DWORD style, DWORD exStyle, int32 x, int32 y, int32 dx, int32 dy) : 
		splitter(_splitter)
	{
		auto hInstance = (HINSTANCE)GetWindowLongPtrA(_splitter.hContainerWnd, GWLP_HINSTANCE);

		WINDOWINFO info;
		info.cbSize = sizeof(info);
		if (!GetWindowInfo(splitter.hContainerWnd, &info))
		{
			Throw(GetLastError(), "CuteChild::GetWindowInfo(...) failed");
		}

		cstr wc = (cstr)info.atomWindowType;
		hChildWnd = CreateWindowExA(exStyle, wc, "", style, x, y, dx, dy, splitter.hContainerWnd, NULL, hInstance, NULL);
		if (hChildWnd == NULL)
		{
			Throw(GetLastError(), "CuteChild::CreateWindowExA(...) failed");
		}

		SetWindowLongPtrA(hChildWnd, GWLP_USERDATA, (LONG_PTR)this);
		SetWindowLongPtrA(hChildWnd, GWLP_WNDPROC, (LONG_PTR)OnSplitterRibbonMessage);
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

IChildSupervisor* CreateSplitterRibbon(Splitter& splitter, DWORD style, DWORD exStyle, int32 x, int32 y, int32 dx, int32 dy)
{
	return new SplitterRibbon(splitter, style, exStyle, x, y, dx, dy);
}

namespace Rococo
{
	namespace Cute
	{
		ISplitSupervisor* CreateSplit(ATOM atom, IParentWindow& parent, int32 pixelSplit, int32 minLo, int32 maxHi, int32 splitterWidth, boolean32 draggable, bool isLeftAndRight, Post::IPostbox& post)
		{
			auto* s = new Splitter(atom, parent, pixelSplit, splitterWidth, minLo, maxHi, draggable, isLeftAndRight, post);
			SetMasterProc(s->Handle(), s);
			return s;
		}
	}
}