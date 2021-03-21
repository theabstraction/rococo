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
			Vec2i span = Widgets::GetSpan(widget);
			GuiRect rect{ 0,0,span.x,span.y };

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