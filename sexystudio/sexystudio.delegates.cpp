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

		void Layout(Rococo::Windows::IWindow& window) override
		{
			HWND hWndParent = GetParent(window);
			if (!IsWindow(hWndParent)) Throw(0, "%s: Bad parent", __FUNCTION__);

			RECT win32rect;
			GetClientRect(window, &win32rect);

			auto& guiRect = reinterpret_cast<GuiRect&>(win32rect);

			for (auto* l : layouts)
			{
				HWNDProxy parent(hWndParent);
				l->Layout(parent, guiRect);
			}
			
			int32 width = guiRect.right - guiRect.left;
			int32 height = guiRect.bottom - guiRect.top;

			MoveWindow(window, guiRect.left, guiRect.top, width, height, TRUE);
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