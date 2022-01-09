#include "sexystudio.impl.h"
#include <rococo.strings.h>
#include <vector>


using namespace Rococo;
using namespace Rococo::Events;
using namespace Rococo::SexyStudio;

namespace
{
	auto evIdToolTip = "sexystudio.tooltip"_event;

	struct TabData
	{
		ITab* tab;
		RECT rect = { -1,-1,-1,-1 };

		TabData(ITab* _tab):
			tab(_tab)
		{
			tab->AddRef();
		}
	};

	struct Tab : ITab, IWin32WindowMessageLoopHandler
	{
		HString name;
		HString tooltip;
		int refCount = 1;
		AutoFree<IWidgetSetSupervisor> children;
		Win32ChildWindow window;

		Tab(IWindow& parent, WidgetContext& context):
			window(parent, *this)
		{
			children = CreateDefaultWidgetSet(window, context);
		}

		LRESULT ProcessMessage(UINT msg, WPARAM wParam, LPARAM lParam)
		{
			switch (msg)
			{
			case WM_ERASEBKGND:
				return TRUE;
			}
			return DefWindowProc(window, msg, wParam, lParam);
		}

		int64 AddRef() override
		{
			return refCount++;
		}

		int64 Release() override
		{
			int64 newCount = refCount--;
			if (newCount == 0)
			{
				delete this;
			}
			return newCount;
		}

		cstr Name() const  override
		{
			return name;
		}

		cstr Tooltip() const  override
		{
			return tooltip;
		}

		void SetName(cstr name) override
		{
			this->name = name;
			SetWindowTextA(window, name);
		}

		void SetTooltip(cstr tooltip) override
		{
			this->tooltip = tooltip;
		}

		void Activate() override
		{
			Layout();
			ShowWindow(window, SW_SHOW);
		}

		void Deactivate() override
		{
			ShowWindow(window, SW_HIDE);
		}

		void Layout() override
		{
			Widgets::ExpandToFillParentSpace(window);
			
			for (auto* child : *children)
			{
				child->Layout();
			}
		}

		IWidgetSet& Children() override
		{
			return *children;
		}
	};

	struct TabHeader : IWin32WindowMessageLoopHandler, IWin32Painter
	{
		Win32ChildWindow window;
		HFONT hTabFont;
		COLORREF tabBkColor;
		COLORREF tabPen;
		IPublisher& publisher;
		Theme& theme;
		std::vector<TabData>& tabs;

		int tabFocus = 0;
		int lastHoverIndex = -1;
		OS::ticks firstHoverTime = 0;

		TabHeader(IWindow& parent, HFONT _hTabFont, COLORREF _tabBkColour, IPublisher& _publisher, Theme& _theme, COLORREF _tabPen, std::vector<TabData>& _tabs):
			window(parent, *this), hTabFont(_hTabFont), tabBkColor(_tabBkColour), publisher(_publisher), theme(_theme), tabPen(_tabPen), tabs(_tabs)
		{
			SetWindowTextA(window, "TabHeader");
		}

		int32 Height() const
		{
			HDC dc = GetDC(window);
			TEXTMETRICA tm;
			GetTextMetricsA(dc, &tm);
			ReleaseDC(window, dc);
			return tm.tmHeight + 2;
		}

		HWND Window()
		{
			return window;
		}

		LRESULT ProcessMessage(UINT msg, WPARAM wParam, LPARAM lParam) override
		{
			switch (msg)
			{
			case WM_PAINT:
				PaintDoubleBuffered(window, *this);
				return 0L;
			case WM_LBUTTONDOWN:
				OnLeftClick(GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam));
				return 0L;
			case WM_MOUSEMOVE:
				InvalidateHeaderRect();
				{
					TRACKMOUSEEVENT me;
					me.cbSize = sizeof TRACKMOUSEEVENT;
					me.dwHoverTime = 0;
					me.hwndTrack = window;
					me.dwFlags = TME_LEAVE;
					TrackMouseEvent(&me);
				}
				OnHover(GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam));
				return 0L;
			case WM_MOUSELEAVE:
				InvalidateHeaderRect();
				{
					TooltipArgs arg;
					arg.text = nullptr;
					arg.useSingleLine = true;
					arg.hoverTime = 0;
					lastHoverIndex = -1;
					publisher.Publish(arg, evIdToolTip);
				}
				return 0L;
			case WM_ERASEBKGND:
				return TRUE;
			}
			return DefWindowProc(window, msg, wParam, lParam);
		}

		RECT DrawTab(ITab& tab, HDC dc, int32 xOffset, bool focus)
		{
			cstr name = tab.Name();
			int32 len = StringLength(name);

			int32 tabWidth = 64;

			DWORD dwExtent = GetTabbedTextExtentA(dc, name, len, 1, &tabWidth);
			uint32 height = HIWORD(dwExtent);
			uint32 width = LOWORD(dwExtent);

			RECT rect;
			rect.left = xOffset;
			rect.right = rect.left + width + 4;
			rect.top = 0;
			rect.bottom = height + 4;

			POINT p;
			GetCursorPos(&p);
			ScreenToClient(window, &p);

			bool isLit = focus || PtInRect(&rect, p);

			HBRUSH hBkBrush = CreateSolidBrush(tabBkColor);
			FillRect(dc, &rect, hBkBrush);
			DeleteObject(hBkBrush);

			auto lastTxColor = SetTextColor(dc, ToCOLORREF(isLit ? theme.normal.txColor : theme.lit.txColor));
			auto lastBk = SetBkColor(dc, ToCOLORREF(theme.normal.bkColor));
			DrawTextA(dc, name, len, &rect, DT_CENTER | DT_VCENTER);
			SetBkColor(dc, lastBk);
			SetBkColor(dc, lastTxColor);

			if (focus)
			{
				if (tabFocus > 0)
				{
					MoveToEx(dc, rect.left, rect.bottom - 1, NULL);
					LineTo(dc, rect.left, rect.top);
					LineTo(dc, rect.right, rect.top);
					LineTo(dc, rect.right, rect.bottom - 1);
				}
				else
				{
					MoveToEx(dc, rect.left, rect.top, NULL);
					LineTo(dc, rect.right, rect.top);
					LineTo(dc, rect.right, rect.bottom - 1);
				}
			}
			else
			{
				MoveToEx(dc, rect.left, rect.bottom - 1, NULL);
				LineTo(dc, rect.right, rect.bottom - 1);
			}

			return rect;
		}

		void DrawHeader(std::vector<TabData>& tabs, HDC dc)
		{
			int x = 0;

			HGDIOBJ oldFont = SelectObject(dc, hTabFont);

			HPEN hPen = CreatePen(PS_SOLID, 1, tabPen);

			HGDIOBJ oldPen = SelectObject(dc, hPen);

			int index = 0;
			int tabBottom = 32;

			for (auto& t : tabs)
			{
				t.rect = DrawTab(*t.tab, dc, x, tabFocus == index);
				x = t.rect.right + 1;
				tabBottom = t.rect.bottom;

				index++;
			}

			RECT rect;
			GetClientRect(window, &rect);

			RECT trailerRect;
			trailerRect.top = 0;
			trailerRect.left = 0;
			trailerRect.right = rect.right;
			trailerRect.bottom = tabBottom;

			if (!tabs.empty())
			{
				trailerRect.left = tabs[tabs.size() - 1].rect.right + 1;
			}

			HBRUSH hTabBrush = CreateSolidBrush(tabBkColor);
			FillRect(dc, &trailerRect, hTabBrush);
			DeleteObject((HGDIOBJ) hTabBrush);

			MoveToEx(dc, x - 1, tabBottom - 1, NULL);
			LineTo(dc, rect.right - 1, tabBottom - 1);

			SelectObject(dc, oldPen);
			SelectObject(dc, oldFont);

			DeleteObject(hPen);
		}

		void OnPaint(HDC dc) override
		{
			DrawHeader(tabs, dc);
		}

		int GetIndexOfTabAtPoint(int x, int y) const
		{
			int index = 0;
			for (auto& t : tabs)
			{
				POINT p{ x, y };
				if (PtInRect(&t.rect, p))
				{
					return index;
				}

				index++;
			}

			return -1;
		}

		void OnLeftClick(int x, int y)
		{
			int index = GetIndexOfTabAtPoint(x, y);
			if (index != -1)
			{
				tabFocus = index;

				int i = 0;
				for (auto& tab : tabs)
				{
					if (i == tabFocus)
					{
						auto& tabWindow = tab.tab->Children().Parent();

						RECT rect;
						GetClientRect(GetParent(tabWindow), &rect);
						MoveWindow(tabWindow, 0, 0, rect.right - rect.left, rect.bottom - rect.top, FALSE);
						tabs[i].tab->Activate();
					}
					else
					{
						tabs[i].tab->Deactivate();
					}
					i++;
				}
			}
		}

		void OnHover(int x, int y)
		{
			int index = GetIndexOfTabAtPoint(x, y);
			if (index != -1)
			{
				if (lastHoverIndex != index)
				{
					lastHoverIndex = index;
					firstHoverTime = Rococo::OS::CpuTicks();

					TooltipArgs arg;
					arg.text = tabs[index].tab->Tooltip();
					arg.useSingleLine = true;
					arg.hoverTime = firstHoverTime;
					arg.backColour = RGBAb(200, 200, 100);
					arg.borderColour = RGBAb(255, 255, 255);
					arg.textColour = RGBAb(0, 0, 0);
					publisher.Publish(arg, evIdToolTip);
				}
			}
			else
			{
				lastHoverIndex = -1;
				TooltipArgs arg;
				arg.text = nullptr;
				arg.useSingleLine = true;
				arg.hoverTime = 0;
				publisher.Publish(arg, evIdToolTip);
			}
		}

		void InvalidateHeaderRect()
		{
			if (!tabs.empty())
			{

				RECT headerRect;
				headerRect.left = tabs[0].rect.left;
				headerRect.top = tabs[0].rect.top;
				headerRect.right = tabs[tabs.size() - 1].rect.right;
				headerRect.bottom = tabs[0].rect.bottom;
				InvalidateRect(window, &headerRect, TRUE);
			}
		}

		void Layout()
		{
			for (auto& tab : tabs)
			{
				tab.tab->Layout();
			}
		}
	};

	struct InvisibleWindow: IWin32WindowMessageLoopHandler, IWindow
	{
		Win32ChildWindow window;

		LRESULT ProcessMessage(UINT msg, WPARAM wParam, LPARAM lParam)
		{
			switch (msg)
			{
			case WM_ERASEBKGND:
				return TRUE;
			}
			return DefWindowProc(window, msg, wParam, lParam);
		}

		InvisibleWindow(InvisibleWindow& src) = delete;
		InvisibleWindow(HWND hParentWnd) : window(hParentWnd, *this) {}

		operator HWND() const { return window; }
	};

	struct TabSplitter : ITabSplitter, IWin32WindowMessageLoopHandler, IWin32Painter
	{
		IWidgetSet& widgets;
		Win32ChildWindow window;
		std::vector<TabData> tabs;
		COLORREF tabBkColour;
		COLORREF tabBkColourHi;
		COLORREF tabPen;
		Theme theme;
		HFONT hTabFont = nullptr;
		TabHeader* header;
		InvisibleWindow tabMainWindow;

		TabSplitter(IWidgetSet& _widgets):
			widgets(_widgets),
			window(_widgets.Parent(), *this),
			tabMainWindow((HWND) window)
		{
			SetWindowTextA(window, "TabSplitter");
			SetWindowTextA(tabMainWindow, "TabMainWindow");

			theme = GetTheme(widgets.Context().publisher);
			tabBkColour = ToCOLORREF(theme.normal.bkColor);
			tabBkColourHi = ToCOLORREF(theme.lit.bkColor);
			tabPen = ToCOLORREF(theme.normal.edgeColor);

			LOGFONTA logFont = { 0 };
			logFont.lfHeight = -12;
			logFont.lfCharSet = ANSI_CHARSET;
			logFont.lfOutPrecision = OUT_DEFAULT_PRECIS;
			logFont.lfClipPrecision = CLIP_DEFAULT_PRECIS;
			logFont.lfQuality = CLEARTYPE_QUALITY;
			SafeFormat(logFont.lfFaceName, "Consolas");

			hTabFont = CreateFontIndirectA(&logFont);

			header = new TabHeader(window, hTabFont, tabBkColour, _widgets.Context().publisher, theme, tabPen, tabs);
		}

		virtual ~TabSplitter()
		{
			if (hTabFont)
			{
				DeleteObject((HGDIOBJ)hTabFont);
			}

			for (auto& t : tabs)
			{
				if (t.tab)
				{
					t.tab->Release();
				}
			}

			delete header;
		}

		ITab& AddTab() override
		{
			tabs.push_back(new Tab(tabMainWindow, widgets.Context()));
			return *tabs.back().tab;
		}

		void OnPaint(HDC dc) override
		{
			RECT rect;
			GetClientRect(window, &rect);

			HBRUSH hBackBrush = CreateSolidBrush(tabBkColour);
			FillRect(dc, &rect, hBackBrush);
			DeleteObject(hBackBrush);
			DrawEdge(dc, &rect, BDR_RAISEDINNER | BDR_RAISEDOUTER, 0);
		}

		LRESULT ProcessMessage(UINT msg, WPARAM wParam, LPARAM lParam) override
		{
			switch (msg)
			{
			case WM_PAINT:
				if (!tabs.empty())
				{
					PAINTSTRUCT ps;
					HDC dc = BeginPaint(window, &ps);
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

			return DefWindowProc(window, msg, wParam, lParam);
		}

		void Layout() override
		{
			RECT rect;
			GetClientRect(GetParent(window), &rect);
			MoveWindow(window, 0, 0, rect.right - rect.left, rect.bottom - rect.top, TRUE);

			int32 headerDY = header->Height();

			MoveWindow(header->Window(), 0, 0, rect.right - rect.left, headerDY, TRUE);
			MoveWindow(tabMainWindow, 0, headerDY, rect.right - rect.left, rect.bottom - headerDY, TRUE);
			
			for (auto& tab : tabs)
			{
				tab.tab->Layout();
			}

			header->Layout();
		}

		void AddLayoutModifier(ILayout* l) override
		{
			Throw(0, "Not implemented");
		}

		IWindow& Window() override
		{
			return window;
		}

		void Free() override
		{
			delete this;
		}

		void SetVisible(bool isVisible) override
		{
			ShowWindow(window, isVisible ? SW_SHOW : SW_HIDE);
			ShowWindow(header->Window(), isVisible ? SW_SHOW : SW_HIDE);
			ShowWindow(tabMainWindow, isVisible ? SW_SHOW : SW_HIDE);
		}

		IWidgetSet* Children() override
		{
			return nullptr;
		}
	};
}

namespace Rococo::SexyStudio
{
	ITabSplitter* CreateTabSplitter(IWidgetSet& widgets)
	{
		auto* tabs = new TabSplitter(widgets);
		widgets.Add(tabs);
		return tabs;
	}
}