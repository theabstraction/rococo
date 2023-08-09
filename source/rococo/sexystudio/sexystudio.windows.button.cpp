#include "sexystudio.impl.h"
#include <rococo.strings.h>

using namespace Rococo;
using namespace Rococo::SexyStudio;

namespace
{
	struct Button : IButtonWidget, IWin32WindowMessageLoopHandler, IWin32Painter
	{
		HBRUSH hBackBrush;
		HBRUSH hHiBrush;

		WidgetContext& context;
		Win32ChildWindow eventSinkWindow;

		IWidgetSet& widgets;
		HString text;

		Rococo::Events::EventIdRef evOnClick;

		AutoFree<ILayoutSet> layouts = CreateLayoutSet();

		HICON icon = nullptr;
		Vec2i iconSpan{ 0,0 };

		Button(WidgetContext& _context, IWidgetSet& _widgets, int16 resourceId, Rococo::Events::EventIdRef _evOnClick):
			hBackBrush(CreateSolidBrush(RGB(192,192,192))),
			hHiBrush(CreateSolidBrush(RGB(224, 224, 224))),
			context(_context),
			eventSinkWindow(_widgets.Parent(), *this),
			widgets(_widgets),
			evOnClick(_evOnClick)
		{
			icon = (HICON)LoadImageA(GetMainInstance(), MAKEINTRESOURCEA(resourceId), IMAGE_ICON, 0, 0, LR_SHARED);
			
			if (icon == nullptr)
			{
				Throw(GetLastError(), "Error loading icon with resource id %d", resourceId);
			}

			ICONINFO iconInfo;
			GetIconInfo(icon, &iconInfo);
			HDC dc = GetDC(eventSinkWindow);
			BITMAPINFO info = { 0 };
			info.bmiHeader.biSize = sizeof(info);

			GetDIBits(dc, iconInfo.hbmColor, 0, 0, nullptr, &info, DIB_RGB_COLORS);

			iconSpan = { info.bmiHeader.biWidth, info.bmiHeader.biHeight };

			if (iconSpan.x > 40 || iconSpan.y > 40)
			{
				Throw(GetLastError(), "Icon with resource id %d was too large. 40x40 pixels is the maximum", resourceId);
			}

			MoveWindow(eventSinkWindow, 0, 0, 40, 40, TRUE);
			ShowWindow(eventSinkWindow, SW_SHOW);

			_widgets.Add(this);
		}
		
		~Button()
		{
			DeleteObject(hHiBrush);
			DeleteObject(hBackBrush);
		}

		void AddLayoutModifier(ILayout* l) override
		{
			layouts->Add(l);
		}

		void Free() override
		{
			delete this;
		}

		int layoutHeight = 0;

		// Specify a layout height, for parents that modify their children's layout
		void SetDefaultHeight(int height)
		{
			layoutHeight = height;
		}

		// return a layout height. If unknown the result is <= 0
		int GetDefaultHeight() const
		{
			return layoutHeight;
		}

		void Layout() override
		{
			layouts->Layout(*this);
		}

		IWindow& Window() override
		{
			return eventSinkWindow;
		}

		void OnClick()
		{ 
			Rococo::Events::TEventArgs<ButtonClickContext> args;
			args.value.sourceWidget = this;
			context.publisher.Publish(args, evOnClick);
		}

		void SetVisible(bool isVisible) override
		{
			ShowWindow(eventSinkWindow, isVisible ? SW_SHOW : SW_HIDE);
			if (isVisible) InvalidateRect(eventSinkWindow, NULL, TRUE);
		}

		bool IsMouseInRect()
		{
			RECT rect;
			GetWindowRect(eventSinkWindow, &rect);
			POINT p;
			GetCursorPos(&p);
			return PtInRect(&rect, p);
		}

		bool isPressing = false;

		void OnPaint(HDC dc) override
		{
			RECT rect;
			GetClientRect(eventSinkWindow, &rect);

			FillRect(dc, &rect, (HBRUSH)COLOR_WINDOW);

			int x = (rect.right - iconSpan.x) >> 1;
			int y = (rect.bottom - iconSpan.y) >> 1;

			if (isPressing)
			{
				x += 1;
				y += 1;
			}

			DrawIcon(dc, x, y, icon);

			if (IsMouseInRect())
			{
				DrawEdge(dc, &rect, isPressing ? EDGE_BUMP : BDR_RAISEDINNER, BF_RECT);
			}
		}

		LRESULT ProcessMessage(UINT msg, WPARAM wParam, LPARAM lParam)
		{
			switch (msg)
			{
			case WM_LBUTTONDOWN:
			{
				InvalidateRect(eventSinkWindow, NULL, FALSE);
				SetCapture(eventSinkWindow);
				isPressing = true;
				return 0L;
			}
			case WM_LBUTTONUP:
			{
				InvalidateRect(eventSinkWindow, NULL, FALSE);
				ReleaseCapture();
				if (isPressing && IsMouseInRect()) OnClick();
				isPressing = false;
				return 0L;
			}
			
			case WM_MOUSEMOVE:
			{
				if (isPressing) return FALSE;

				TRACKMOUSEEVENT tme;
				tme.cbSize = sizeof tme;
				tme.dwHoverTime = 0;
				tme.hwndTrack = eventSinkWindow;
				tme.dwFlags = TME_LEAVE;
				TrackMouseEvent(&tme);
				InvalidateRect(eventSinkWindow, NULL, FALSE);
				return FALSE;
			}
			case WM_MOUSELEAVE:
			{
				InvalidateRect(eventSinkWindow, NULL, FALSE);
				return FALSE;
			}
			case WM_ERASEBKGND:
				return TRUE;
			case WM_PAINT:
			{
				PaintDoubleBuffered(eventSinkWindow, *this);
				return 0L;
			}
			break;
			}
			return DefWindowProc(eventSinkWindow, msg, wParam, lParam);
		}

		IWidgetSet* Children() override
		{
			return nullptr;
		}
	};
}

namespace Rococo::SexyStudio
{
	IButtonWidget* CreateButtonByResource(WidgetContext& context, IWidgetSet& widgets, int16 resourceId, Rococo::Events::EventIdRef evOnClick)
	{
		return new Button(context, widgets, resourceId, evOnClick);
	}
}