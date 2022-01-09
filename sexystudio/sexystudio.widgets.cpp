#include "sexystudio.impl.h"
#include <vector>

#include <rococo.maths.h>

using namespace Rococo;
using namespace Rococo::SexyStudio;

namespace
{
	struct DefaultWidgetSet : IWidgetSetSupervisor
	{
		WidgetContext& context;
		Rococo::Windows::IWindow& parent;
		std::vector<IGuiWidget*> widgets;

		DefaultWidgetSet(Rococo::Windows::IWindow& _parent, WidgetContext& _context):
			context(_context),
			parent(_parent)
		{

		}

		~DefaultWidgetSet()
		{
			for (auto* widget : widgets)
			{
				widget->Free();
			}
		}

		void Add(IGuiWidget* widget) override
		{
			widgets.push_back(widget);
		}

		void Free() override
		{
			delete this;
		}

		IGuiWidget** begin() override
		{
			return widgets.empty() ? nullptr : &widgets[0];
		}

		IGuiWidget** end() override
		{
			return widgets.empty() ? nullptr : &widgets[0] + widgets.size();
		}
		 
		Rococo::Windows::IWindow& Parent() override
		{
			return parent;
		}

		WidgetContext& Context() override
		{
			return context;
		}
	};
}

namespace Rococo::SexyStudio
{
	void SetPosition(IWindow& window, const GuiRect& rect)
	{
		MoveWindow(window, rect.left, rect.top, Width(rect), Height(rect), TRUE);
	}

	Vec2i GetSpan(IWindow& window)
	{
		RECT rect;
		GetClientRect(window, &rect);
		return { rect.right - rect.left, rect.bottom - rect.top };
	}

	IWidgetSetSupervisor* CreateDefaultWidgetSet(Rococo::Windows::IWindow& parent, WidgetContext& context)
	{
		return new DefaultWidgetSet(parent, context);
	}
}