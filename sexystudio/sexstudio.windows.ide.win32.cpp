#include "sexystudio.impl.h"
#include <rococo.strings.h>
#include <rococo.sexystudio.api.h>
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

		Time::ticks lastHoverTime = 0;

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

	enum { WM_PROGRESS_HIDE = WM_USER };

	class Win32ProgressWindow : private IWin32WindowMessageLoopHandler, IWin32Painter
	{
		WidgetContext wc;
		Win32PopupWindow window;
		HString banner;
		float percent = 0;

	public:
		Win32ProgressWindow(HWND hParent, WidgetContext& _wc): wc(_wc), window(hParent, *this)
		{

		}

		void DrawRect(const RECT& rect, HDC dc, COLORREF colour)
		{
			HBRUSH hBrush = CreateSolidBrush(colour);
			FillRect(dc, &rect, hBrush);
			DeleteObject((HGDIOBJ)hBrush);
		}

		void OnPaint(HDC dc) override
		{
			auto oldFont = SelectObject(dc, wc.fontSmallLabel);

			RECT rect;
			GetClientRect(window, &rect);

			COLORREF backColour = RGB(160, 160, 200);

			DrawRect(rect, dc, backColour);

			SetBkColor(dc, backColour);
			SetTextColor(dc, RGB(0, 0, 0));

			int xBorder = 4;

			RECT bannerRect = rect;
			bannerRect.left += xBorder;
			bannerRect.right -= xBorder;
			bannerRect.top += 20;
			bannerRect.bottom = 200;

			DrawTextA(dc, banner, -1, &bannerRect, DT_LEFT | DT_TOP);

			int yCenter = rect.bottom / 2;

			RECT progressBar = rect;
			progressBar.left += xBorder;
			progressBar.right -= xBorder;
			progressBar.top = yCenter - 20;
			progressBar.bottom = yCenter + 20;

			SelectObject(dc, GetStockObject(BLACK_PEN));

			POINT p;
			MoveToEx(dc, progressBar.left, progressBar.top, &p);
			LineTo(dc, progressBar.right, progressBar.top);
			LineTo(dc, progressBar.right, progressBar.bottom);
			LineTo(dc, progressBar.left, progressBar.bottom);
			LineTo(dc, progressBar.left, progressBar.top);

			progressBar.left += 1;
			progressBar.right -= 1;
			progressBar.top += 1;
			progressBar.bottom -= 1;

			RECT progressTextBar = progressBar;

			int span = (int) ((progressBar.right - progressBar.left) * (percent / 100.0f));

			progressBar.right = progressBar.left + span;

			COLORREF barColour = RGB(0, 0, 255);

			DrawRect(progressBar, dc, barColour);

			SetBkColor(dc, RGB(255,255,255));

			int xCenter = rect.right / 2;

			progressTextBar.left = xCenter - 30;
			progressTextBar.right = xCenter + 30;
			progressTextBar.top += 8;
			progressTextBar.bottom -= 8;

			DrawRect(progressTextBar, dc, RGB(255, 255, 255));

			char buf[32];
			SafeFormat(buf, "%2.0f %%", percent);
			DrawTextA(dc, buf, -1, &progressTextBar, DT_SINGLELINE | DT_CENTER | DT_VCENTER);

			SelectObject(dc, oldFont);
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
			case WM_PROGRESS_HIDE:
				ShowWindow(window, SW_HIDE);
				return 0L;
			}
			return DefWindowProc(window, msg, wParam, lParam);
		}

		void SetProgress(float progressPercent, cstr bannerText)
		{
			if (!IsWindowVisible(window))
			{
				Layout();
			}

			banner = bannerText;
			percent = clamp(progressPercent, 0.0f, 100.0f);

			ShowWindow(window, SW_SHOW);
			if (progressPercent >= 100.0f)
			{
				PostMessage(window, WM_PROGRESS_HIDE, 0, 0);
			}

			InvalidateRect(window, NULL, FALSE);
			UpdateWindow(window);
		}

		operator HWND() { return window; }

		void Layout()
		{
			RECT parentRect;
			if (GetClientRect(GetParent(window), &parentRect))
			{
				Vec2i span = { 600, 300 };
				int x = (parentRect.right -  span.x) / 2;
				int y = (parentRect.bottom - span.y) / 2;

				POINT p = { x,y };
				ClientToScreen(GetParent(window), &p);
				
				MoveWindow(window, p.x, p.y, span.x, span.y, FALSE);
			}
		}
	};

	class Win32MainIDEWindow : public IIDEFrameSupervisor, private IWin32WindowMessageLoopHandler, public IWin32Painter
	{
	public:
		WidgetContext& context;
		Win32TopLevelWindow mainFrame;
		Win32ProgressWindow progressWindow;
		TooltipWindow tooltipWindow;
		AutoFree<IWidgetSetSupervisor> children;
		EventIdRef evClose = { 0 };
		EventIdRef evResize = { 0 };
		ISexyStudioEventHandler& eventHandler;

		Win32MainIDEWindow(WidgetContext& _context, IWindow& topLevelWindow, ISexyStudioEventHandler& evHandler) :
			context(_context),
			mainFrame(ideExStyle, ideStyle, *this, topLevelWindow),
			tooltipWindow(mainFrame, context),
			progressWindow((HWND) mainFrame, _context),
			eventHandler(evHandler)
		{
			children = CreateDefaultWidgetSet(mainFrame, context);
		}

		ISexyStudioEventHandler& Events()
		{
			return eventHandler;
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
			case WM_GETMINMAXINFO:
				{
					auto* m = (MINMAXINFO*)lParam;
					m->ptMinTrackSize = { 800, 600 };
				}
				return 0;
			case WM_ERASEBKGND:
				return TRUE;
			case WM_CLOSE:
				if (evClose.name == nullptr)
				{
					auto result = eventHandler.OnIDEClose(Window());
					if (result == EIDECloseResponse::Shutdown)
					{
						PostQuitMessage(0);
					}
				}
				else
				{
					Rococo::Events::TEventArgs<ButtonClickContext> args;
					args.value.sourceWidget = nullptr;
					context.publisher.Publish(args, evClose);
				}
				return TRUE;
			case WM_MOVE:
				progressWindow.Layout();
				break;
			case WM_SIZE:
				if (wParam != SIZE_MINIMIZED)
				{
					progressWindow.Layout();
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

		void SetProgress(float progressPercent, cstr bannerText) override
		{
			progressWindow.SetProgress(progressPercent, bannerText);
		}
	};
}

namespace Rococo::SexyStudio
{
	IIDEFrameSupervisor* CreateMainIDEFrame(WidgetContext& context, IWindow& topLevelWindow, ISexyStudioEventHandler& eventHandler)
	{
		return new Win32MainIDEWindow(context, topLevelWindow, eventHandler);
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