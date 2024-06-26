#include "sexystudio.impl.h"
#include <rococo.strings.h>
#include <unordered_set>

using namespace Rococo;
using namespace Rococo::Windows;
using namespace Rococo::SexyStudio;

namespace
{
	struct Toolbar: IToolbar, IWin32WindowMessageLoopHandler
	{
		int leftBorder = 0;
		int widgetGap = 0;

		Win32ChildWindow backgroundWindow;

		AutoFree<ILayoutSet> layouts = CreateLayoutSet();
		AutoFree<IWidgetSetSupervisor> children;

		std::unordered_set<IGuiWidget*> manualLayouts;

		Toolbar(IWidgetSet& widgets) : backgroundWindow(widgets.Parent(), *this),
			children(CreateDefaultWidgetSet(backgroundWindow, widgets.Context()))
		{
		}

		~Toolbar()
		{
		}

		Rococo::Windows::IWindow& Window() override
		{
			return backgroundWindow;
		}

		void Free() override
		{
			delete this;
		}

		IWidgetSet* Children() override
		{
			return children;
		}

		void AddLayoutModifier(ILayout* l) override
		{
			layouts->Add(l);
		}

		void Layout()
		{
			layouts->Layout(*this);

			int x = leftBorder;

			Vec2i toolbarSpan = GetSpan(backgroundWindow);

			for (IGuiWidget* widget : *children)
			{
				if (manualLayouts.find(widget) == manualLayouts.end())
				{
					Vec2i span = GetSpan(widget->Window());

					int topBorder = (toolbarSpan.y - span.y) >> 1;

					GuiRect rect;
					rect.left = x;
					rect.right = x + span.x;
					rect.top = topBorder;
					rect.bottom = topBorder + span.y;

					SetPosition(widget->Window(), rect);
					
					x += span.x + widgetGap;
				}
				else
				{
					widget->Layout();
				}
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

		void SetSpacing(int32 firstBorder, int32 widgetSpacing)
		{
			this->leftBorder = firstBorder;
			this->widgetGap = widgetSpacing;
		}

		LRESULT ProcessMessage(UINT msg, WPARAM wParam, LPARAM lParam) override
		{
			switch (msg)
			{
			case WM_IDE_RESIZED:
				Layout();
				return 0L;
			default:
				break;
			}

			return DefWindowProc(backgroundWindow, msg, wParam, lParam);
		}

		void SetVisible(bool isVisible) override
		{
			if (isVisible)
			{
				Layout();
			}
			ShowWindow(backgroundWindow, isVisible ? SW_SHOW : SW_HIDE);
		}

		void SetManualLayout(IGuiWidget* widget) override
		{
			manualLayouts.insert(widget);
		}
	};
}

namespace Rococo::SexyStudio
{
	IToolbar* CreateToolbar(IWidgetSet& widgets)
	{
		return new Toolbar(widgets);
	}
}