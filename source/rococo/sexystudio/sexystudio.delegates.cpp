#include "sexystudio.impl.h"
#include <vector>
#include <rococo.strings.h>

using namespace Rococo;
using namespace Rococo::SexyStudio;
using namespace Rococo::Strings;

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
			GuiRect rect = { -1, -1, -1, -1 };

			for (auto* l : layouts)
			{
				l->Layout(widget, rect);
			}

			if (rect.left < 0 || rect.right <= rect.left || rect.top < 0 || rect.bottom <= rect.top)
			{
				char ancestors[1024];
				StackStringBuilder sb(ancestors, sizeof ancestors);
				AppendAncestorsToString(widget, sb);
				Throw(0, "%s: rect not properly defined: { %d, %d, %d, %d }.\r\nWindow hierarchy:\r\n%s", __FUNCTION__, rect.left, rect.top, rect.right, rect.bottom, ancestors);
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