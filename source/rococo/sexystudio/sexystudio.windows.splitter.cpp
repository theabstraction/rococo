#include "sexystudio.impl.h"
#include <rococo.strings.h>
#include <rococo.maths.h>


using namespace Rococo;
using namespace Rococo::Events;
using namespace Rococo::SexyStudio;
using namespace Rococo::Windows;

namespace
{
	ROCOCO_INTERFACE IDragEvents
	{
		virtual void OnBeginDrag() = 0;
		virtual void OnDrag() = 0;
		virtual void OnEndDrag() = 0;
	};

	struct SplitDragger : IGuiWidget, IWin32WindowMessageLoopHandler, IWindow, IWin32Painter
	{
		IDragEvents& dragEventsSink;
		Win32ChildWindow window;

		Brush bkBrush;
		Brush bkBrushHi;

		SplitDragger(IWidgetSet& widgets, IDragEvents& _dragEventSink) :
			window(widgets.Parent(), *this),
			dragEventsSink(_dragEventSink)
		{
			bkBrush = RGB(64, 64, 64);
			bkBrushHi = RGB(96, 96, 96);

			widgets.Add(this);
		}

		void OnPaint(HDC dc) override
		{
			RECT rect;
			GetClientRect(window, &rect);

			bool isLit = isDragging;

			FillRect(dc, &rect, isLit ? bkBrushHi : bkBrush);
			DrawEdge(dc, &rect, isLit ? BDR_SUNKENINNER | BDR_SUNKENOUTER : BDR_RAISEDINNER | BDR_RAISEDOUTER, 0);
		}

		POINT dragStart = { 0,0 };
		POINT dragCurrent = { 0,0 };
		bool isDragging = false;

		LRESULT ProcessMessage(UINT msg, WPARAM wParam, LPARAM lParam)
		{
			switch (msg)
			{
			case WM_PAINT:
				PaintDoubleBuffered(window, *this);
				return 0L;
			case WM_ERASEBKGND:
				return TRUE;
			case WM_LBUTTONDOWN:
				GetCursorPos(&dragStart);
				SetCapture(window);
				dragEventsSink.OnBeginDrag();
				isDragging = true;
				return TRUE;
			case WM_LBUTTONUP:
				ReleaseCapture();
				isDragging = false;
				dragEventsSink.OnEndDrag();
				return TRUE;
			case WM_MOUSEMOVE:
				GetCursorPos(&dragCurrent);
				if (isDragging)
				{
					dragEventsSink.OnDrag();
					break;
				}

				InvalidateRect(window, NULL, TRUE);
				{
					TRACKMOUSEEVENT me;
					me.cbSize = sizeof TRACKMOUSEEVENT;
					me.dwHoverTime = 0;
					me.hwndTrack = window;
					me.dwFlags = TME_LEAVE;
					TrackMouseEvent(&me);
				}
				break;
			case WM_MOUSELEAVE:
				InvalidateRect(window, NULL, TRUE);
				break;
			case WM_SETCURSOR:
				SetCursor(LoadCursorA(NULL, IDC_SIZEWE));
				return 0L;
			}
			return DefWindowProcA(window, msg, wParam, lParam);
		}

		void Layout() override
		{
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

		void AddLayoutModifier(ILayout*) override
		{
			Throw(0, "%s: Not permitted", __FUNCTION__);
		}

		operator HWND() const override
		{
			return window;
		}

		IWindow& Window() override
		{
			return *this;
		}

		void Free() override
		{
			delete this;
		}

		void SetVisible(bool isVisible) override
		{
			ShowWindow(window, isVisible ? SW_SHOW : SW_HIDE);
		}

		IWidgetSet* Children() override
		{
			return nullptr;
		}
	};

	bool HasChild(HWND hWnd)
	{
		struct ANON
		{
			static BOOL MarkFound(HWND, LPARAM param)
			{
				bool& found = *(bool*)param;
				found = true;
				return FALSE;
			}
		};
		
		bool found = false;
		EnumChildWindows(hWnd, ANON::MarkFound, (LPARAM) &found);
		return found;
	}

	struct SplitScreen : ISplitScreen, IWin32WindowMessageLoopHandler, IWindow, IWin32Painter, IDragEvents
	{
		ISplitScreen* firstHalf = nullptr;
		ISplitScreen* secondHalf = nullptr;
		SplitDragger* dragger = nullptr;

		Win32ChildWindow window;

		AutoFree<IWidgetSetSupervisor> children;
		AutoFree<ILayoutSet> layoutRules;

		enum class ESplitDirection
		{
			None,
			Rows,
			Columns
		} splitAs = ESplitDirection::None;

		int32 splitPos = 0;
		int32 shadowSplitPos = 0;

		Brush bkBrush;

		Theme theme;

		// Recursively generated split screens manage their own layouts and so do not need anchors
		// However the container for the recursive set needs to define how to arrange the outer set using anchors
		bool useAnchors;

		SplitScreen(IWidgetSet& widgets, bool _useAnchors):
			window(widgets.Parent(), *this), useAnchors(_useAnchors)
		{
			children = CreateDefaultWidgetSet(window, widgets.Context());
			theme = GetTheme(widgets.Context().publisher);
			bkBrush = ToCOLORREF(theme.normal.bkColor);
			layoutRules = CreateLayoutSet();

			char windowName[256];
			SafeFormat(windowName, "Splitter 0x%llx", (HWND)window);
			SetWindowTextA(window, windowName);
		}

		void OnPaint(HDC dc) override
		{
			RECT rect;
			GetClientRect(window, &rect);
			FillRect(dc, &rect, bkBrush);
			DrawEdge(dc, &rect, BDR_RAISEDINNER | BDR_RAISEDOUTER, 0);
		}

		void OnBeginDrag() override
		{
			shadowSplitPos = splitPos;
		}

		void OnDrag() override
		{
			splitPos = shadowSplitPos + dragger->dragCurrent.x - dragger->dragStart.x;
			Layout();
		}

		void OnEndDrag() override
		{
			shadowSplitPos = 0;
		}

		LRESULT ProcessMessage(UINT msg, WPARAM wParam, LPARAM lParam)
		{
			switch (msg)
			{
			case WM_PAINT:
				if (HasChild(window))
				{
					PAINTSTRUCT ps;
					HDC dc = BeginPaint(window, &ps);
					UNUSED(dc);
					EndPaint(window, &ps);
				}
				else
				{
					PaintDoubleBuffered(window, *this);
				}
				return 0L;
			case WM_ERASEBKGND:
				return TRUE; 
			}
		
			return DefWindowProcA(window, msg, wParam, lParam);
		}

		ISplitScreen* GetFirstHalf() override
		{
			return firstHalf;
		}

		ISplitScreen* GetSecondHalf()  override
		{
			return secondHalf;
		}

		GuiRect GetRect()  override
		{
			auto screenRect = Widgets::GetScreenRect(*this);
			return Widgets::MapScreenToWindowRect(screenRect, *this);
		}

		void Split()
		{
			if (!firstHalf)
			{
				firstHalf = new SplitScreen(*children, false);
				children->Add(firstHalf);

				secondHalf = new SplitScreen(*children, false);
				children->Add(secondHalf);

				dragger = new SplitDragger(*children, *this);
			}
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

		void SplitIntoColumns(int32 firstSpan) override
		{
			Split();
			splitAs = ESplitDirection::Columns;
			splitPos = firstSpan;
		}

		void SplitIntoRows(int32 firstSpan) override
		{
			Split();
			splitAs = ESplitDirection::Rows;
			splitPos = firstSpan;
		}

		void Merge()  override
		{

		}

		void SantizeSplitPosition()
		{
			if (splitPos < 100)
			{
				splitPos = 100;
			}

			RECT rect;
			GetClientRect(window, &rect);
			if (splitPos > rect.right - 100)
			{
				splitPos = rect.right - 100;
			}
		}

		void Layout() override
		{
			if (useAnchors) layoutRules->Layout(*this);

			RECT rect;
			GetClientRect(GetParent(window), &rect);

			if (firstHalf)
			{
				SantizeSplitPosition();
				Vec2i spanA;
				Vec2i spanB;
				Vec2i a, b;

				int32 draggerWidth = 8;

				Vec2i draggerPos;
				Vec2i draggerSpan;

				switch (splitAs)
				{
				case ESplitDirection::Columns:
					a = { 0, 0 };
					b = { splitPos + draggerWidth, 0 };
					spanA = { splitPos, rect.bottom - rect.top };
					spanB = { rect.right - rect.left - b.x, rect.bottom - rect.top };
					draggerPos = { spanA.x, 0 };
					draggerSpan = { draggerWidth, spanA.y };
					break;
				default:  // rows
					a = { 0, 0 };
					b = { 0, splitPos + draggerWidth };
					spanA = { rect.right - rect.left, splitPos };
					spanB = { rect.right - rect.left, rect.bottom - rect.top - b.y};
					draggerPos = { 0, spanA.y };
					draggerSpan = { spanA.x, draggerWidth };
				};

				MoveWindow(firstHalf->Window(), a.x, a.y, spanA.x, spanA.y, FALSE);
				MoveWindow(secondHalf->Window(), b.x, b.y, spanB.x, spanB.y, FALSE);
				MoveWindow(dragger->Window(), draggerPos.x, draggerPos.y, draggerSpan.x, draggerSpan.y, FALSE);
				firstHalf->Layout();
				secondHalf->Layout();
				InvalidateRect(window, NULL, TRUE);
				UpdateWindow(window);
			}
			else
			{
				for (auto* child : *children)
				{
					child->Layout();
				}
			}
		}

		void AddLayoutModifier(ILayout* l) override
		{
			layoutRules->Add(l);
		}

		operator HWND() const override
		{
			return window;
		}

		IWindow& Window() override
		{
			return *this;
		}

		void Free() override
		{
			delete this;
		}

		void SetVisible(bool isVisible) override
		{
			ShowWindow(window, isVisible ? SW_SHOW : SW_HIDE);

			if (firstHalf)
			{
				firstHalf->SetVisible(isVisible);
				secondHalf->SetVisible(isVisible);
				dragger->SetVisible(isVisible);
			}

			InvalidateRect(window, NULL, TRUE);
		}

		IWidgetSet* Children() override
		{
			return children;
		}

		void SetBackgroundColour(RGBAb colour) override
		{
			bkBrush = RGB(colour.red, colour.green, colour.blue);
			InvalidateRect(window, NULL, TRUE);
		}
	};
}

namespace Rococo::SexyStudio
{
	ISplitScreen* CreateSplitScreen(IWidgetSet& widgets)
	{
		auto* ss = new SplitScreen(widgets, true);
		widgets.Add(ss);
		return ss;
	}
}