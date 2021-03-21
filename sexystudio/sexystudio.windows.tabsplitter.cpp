#include "sexystudio.impl.h"
#include <rococo.strings.h>

#include <rococo.events.h>

#include <vector>

using namespace Rococo;
using namespace Rococo::Events;
using namespace Rococo::SexyStudio;

namespace
{
	struct TabData
	{
		ITab* tab;

		TabData(TabDefiniton& tabDef): tab(tabDef.tab)
		{
			tab->AddRef();
		}
	};

	struct Tab : ITab
	{
		HString name;
		HString tooltip;
		int refCount = 1;

		Tab()
		{

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
		}

		void SetTooltip(cstr tooltip) override
		{
			this->tooltip = tooltip;
		}
	};

	struct TabSplitter : ITabSplitter, IWin32WindowMessageLoopHandler
	{
		IPublisher& publisher;
		IWidgetSet& widgets;
		Win32ChildWindow window;
		std::vector<TabData> tabs;
		Brush tabBkColour;

		TabSplitter(IPublisher& _publisher, IWidgetSet& _widgets):
			publisher(_publisher), widgets(_widgets),
			window(_widgets.Parent(), *this)
		{
			tabBkColour.hBrush = CreateSolidBrush(RGB(192,192,192));
		}

		RECT DrawTab(ITab& tab, HDC dc, int32 xOffset)
		{
			cstr name = tab.Name();
			int32 len = StringLength(name);

			int32 tabWidth = 64;

			DWORD dwExtent = GetTabbedTextExtentA(dc, name, len, 1, &tabWidth);
			uint32 height = HIWORD(dwExtent);
			uint32 width = LOWORD(dwExtent);

			RECT rect;
			rect.left = xOffset;
			rect.right = rect.left + width + 12;
			rect.top = 0;
			rect.bottom = height + 4;

			FillRect(dc, &rect, tabBkColour);
			DrawTextA(dc, name, len, &rect, DT_LEFT | DT_VCENTER);

			return rect;
		}

		void DrawHeader(std::vector<TabData>& tabs, HDC dc)
		{
			int x = 0;

			for (auto& t : tabs)
			{
				RECT rect = DrawTab(*t.tab, dc, x);
				x = rect.right + 1;
			}
		}

		void OnPaint(HDC dc)
		{
			DrawHeader(tabs, dc);
		}

		LRESULT ProcessMessage(UINT msg, WPARAM wParam, LPARAM lParam)
		{
			switch (msg)
			{
			case WM_PAINT:
				{
					PAINTSTRUCT ps;
					HDC dc = BeginPaint(window, &ps);
					OnPaint(dc);
					EndPaint(window, &ps);
					return 0L;
				}
			}
			return DefWindowProc(window, msg, wParam, lParam);
		}

		void AddTab(TabDefiniton& tabDef) override
		{
			if (tabDef.tab == nullptr) Throw(0, "%s: tab null", __FUNCTION__);
			tabs.push_back(TabData(tabDef));
		}

		void Layout() override
		{
			RECT rect;
			GetClientRect(GetParent(window), &rect);
			MoveWindow(window, 0, 0, rect.right - rect.left, rect.bottom - rect.top, TRUE);
		}

		void AddLayoutModifier(ILayout* preprocessor) override
		{

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
		}

		IWidgetSet* Children() override
		{
			return nullptr;
		}
	};
}

namespace Rococo::SexyStudio
{
	ITabSplitter* CreateTabSplitter(IPublisher& publisher, IWidgetSet& widgets)
	{
		if (&widgets == nullptr) Throw(0, "%s: Widgets was a null reference.", __FUNCTION__);
		auto* tab = new TabSplitter(publisher, widgets);
		widgets.Add(tab);
		return tab;
	}

	ITab* CreateTab()
	{
		return new Tab();
	}
}