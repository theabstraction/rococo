#include <rococo.gui.retained.h>
#include <rococo.maths.i32.h>

namespace ANON
{
	using namespace Rococo;
	using namespace Rococo::Gui;

	struct GRLayoutSystem_Fullscreen: IGRLayoutSupervisor
	{
		void Free() override
		{
			delete this;
		}

		void Layout(IGRPanel& panel, const GuiRect& screenRect)
		{
			panel.Resize(Span(screenRect));
		}
	};
}

namespace Rococo::Gui
{
	IGRLayoutSupervisor* CreateFullScreenLayout()
	{
		return new ANON::GRLayoutSystem_Fullscreen();
	}
}