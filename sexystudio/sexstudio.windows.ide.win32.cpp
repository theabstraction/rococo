#include "sexystudio.impl.h"
#include <rococo.strings.h>
#include <vector>
#include "resource.h"

using namespace Rococo;
using namespace Rococo::Events;
using namespace Rococo::SexyStudio;

namespace
{
	const DWORD ideExStyle = 0;
	const DWORD ideStyle = WS_OVERLAPPEDWINDOW;

	auto evIdToolTip = "sexystudio.tooltip"_event;

	class TooltipWindow: IWin32WindowMessageLoopHandler, IObserver, IWin32Painter
	{
	public:
		WidgetContext& context;
		Win32PopupWindow window;
		std::vector<char> buffer;

		OS::ticks lastHoverTime = 0;

		COLORREF bkColour = { 0 };
		COLORREF txColour = { 0 };
		COLORREF edgeColour = { 0 };

		Brush brush;
		Pen pen;

		TooltipWindow(HWND hWnd, WidgetContext& _context) :
			context(_context),
			window(hWnd, *this)
		{
			context.publisher.Subscribe(this, evIdToolTip);
		}

		~TooltipWindow()
		{
			context.publisher.Unsubscribe(this);
		}

		void OnEvent(Event& ev) override
		{
			if (ev == evIdToolTip)
			{
				auto& args = As<TooltipArgs>(ev);
				cstr text = args.text;
				if (!text || text[0] == 0)
				{
					buffer.clear();
					ShowWindow(window, SW_HIDE);
					lastHoverTime = 0;
				}
				else
				{
					buffer.resize(strlen(text) + 1);
					memcpy(buffer.data(), text, strlen(text) + 1);
					lastHoverTime = args.hoverTime;

					HDC dc = GetDC(window);
					int tabWidth = 8;
					int len = (int32) buffer.size() - 1;
					DWORD dwExtent = GetTabbedTextExtentA(dc, buffer.data(), len, 1, &tabWidth);
					uint32 height = HIWORD(dwExtent);
					uint32 width = LOWORD(dwExtent);
					ReleaseDC(window, dc);

					bkColour = RGB(args.backColour.red, args.backColour.green, args.backColour.blue);
					txColour = RGB(args.textColour.red, args.textColour.green, args.textColour.blue);
					edgeColour = RGB(args.borderColour.red, args.borderColour.green, args.borderColour.blue);

					brush = bkColour;
					pen = edgeColour;

					POINT p;
					GetCursorPos(&p);

					MoveWindow(window, p.x, p.y + 16, width + 20, 24, TRUE);
					ShowWindow(window, SW_SHOW);

					InvalidateRect(window, NULL, TRUE);
				}
			}
		}

		void OnPaint(HDC dc) override
		{
			RECT rect;
			GetClientRect(window, &rect);
			FillRect(dc, &rect, brush);

			HFONT oldFont = (HFONT) SelectObject(dc, context.fontSmallLabel);

			SetTextColor(dc, txColour);
			SetBkColor(dc, bkColour);
			DrawTextA(dc, buffer.data(), (int32) buffer.size() - 1, &rect, DT_VCENTER | DT_CENTER | DT_SINGLELINE);

			SelectObject(dc, oldFont);

			auto oldPen = SelectObject(dc, pen);
			MoveToEx(dc, rect.left, rect.top, NULL);
			LineTo(dc, rect.right - 1, rect.top);
			LineTo(dc, rect.right - 1, rect.bottom-1);
			LineTo(dc, rect.left, rect.bottom - 1);
			LineTo(dc, rect.left, rect.top);
			SelectObject(dc, oldPen);
		}

		LRESULT ProcessMessage(UINT msg, WPARAM wParam, LPARAM lParam) override
		{
			switch (msg)
			{
			case WM_PAINT:
				PaintDoubleBuffered(window, *this);
				return 0L;
			case WM_ERASEBKGND:
				return TRUE;
			}
			return DefWindowProcA(window, msg, wParam, lParam);
		}
	};

	class Win32MainIDEWindow : public IIDEFrameSupervisor, private IWin32WindowMessageLoopHandler, public IWin32Painter
	{
	public:
		WidgetContext& context;
		Win32TopLevelWindow mainFrame;
		TooltipWindow tooltipWindow;
		AutoFree<IWidgetSetSupervisor> children;
		EventIdRef evClose = { 0 };
		EventIdRef evResize = { 0 };

		Win32MainIDEWindow(WidgetContext& _context) :
			context(_context),
			mainFrame(ideExStyle, ideStyle, *this),
			tooltipWindow(mainFrame, context)
		{
			children = CreateDefaultWidgetSet(mainFrame, context);
		}

		void LayoutChildren() override
		{
			for (auto* child : *children)
			{
				child->Layout();
			}
		}

		void OnPaint(HDC dc) override
		{

		}

		LRESULT ProcessMessage(UINT msg, WPARAM wParam, LPARAM lParam) override
		{
			switch (msg)
			{
			case WM_ERASEBKGND:
				return TRUE;
			case WM_CLOSE:
				if (evClose.name == nullptr)
				{
					PostQuitMessage(0);
				}
				else
				{
					Rococo::Events::TEventArgs<ButtonClickContext> args;
					args.value.sourceWidget = nullptr;
					context.publisher.Publish(args, evClose);
				}
				break;
			case WM_SIZE:
				if (evResize.name != nullptr)
				{
					Rococo::Events::TEventArgs<IWidgetSet*> args;
					args.value = children;
					context.publisher.Publish(args, evResize);
				}
				else
				{
					LayoutChildren();
				}
				return 0L;
			}
			return DefWindowProcA(mainFrame, msg, wParam, lParam);
		}

		IWindow& Window() override
		{
			return mainFrame;
		}

		void Free() override
		{
			delete this;
		}

		RECT borderThickness = { 0 };

		void SetVisible(bool isVisible) override
		{
			ShowWindow(mainFrame, isVisible ? SW_SHOW : SW_HIDE);
			LayoutChildren();
		}

		IWidgetSet& Children() override
		{
			return *children;
		}

		void SetCloseEvent(const Rococo::Events::EventIdRef& evClose) override
		{
			this->evClose = evClose;
		}

		void SetResizeEvent(const Rococo::Events::EventIdRef& evResize) override
		{
			this->evResize = evResize;
		}
	};
}

namespace Rococo::SexyStudio
{
	IIDEFrameSupervisor* CreateMainIDEFrame(WidgetContext& context)
	{
		return new Win32MainIDEWindow(context);
	}

	void UseDefaultFrameBarLayout(IToolbar& frameBar)
	{
		Widgets::AnchorToParentLeft(frameBar, 0);
		Widgets::AnchorToParentRight(frameBar, 0);
		Widgets::AnchorToParentTop(frameBar, 0);
		Widgets::ExpandBottomFromTop(frameBar, 48);
		frameBar.SetSpacing(10, 20);
	}
}