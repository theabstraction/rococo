#include "sexystudio.impl.h"
#include <vector>

using namespace Rococo;
using namespace Rococo::SexyStudio;

namespace
{
	struct Layouts: ILayoutSet
	{
		std::vector<ILayout*> layouts;

		~Layouts()
		{
			for (auto* d : layouts)
			{
				d->Free();
			}
		}

		void Add(ILayout* d) override
		{
			layouts.push_back(d);
		}

		void Free()  override
		{
			delete this;
		}

		void Layout(IGuiWidget& widget) override
		{
			GuiRect rect = Widgets::GetScreenRect(widget);
			rect = Widgets::MapScreenToWindowRect(rect, widget);

			for (auto* l : layouts)
			{
				l->Layout(widget, rect);
			}

			Widgets::SetWidgetPosition(widget, rect);
		}
	};
}

namespace Rococo::SexyStudio
{
	ILayoutSet* CreateLayoutSet()
	{
		return new Layouts();
	}
}