#include <rococo.gui.retained.h>

namespace ANON
{
	using namespace Rococo::Gui;

	struct GRMainFrame: IGRMainFrameSupervisor
	{
		IGRPanel& panel;

		GRMainFrame(IGRPanel& _panel) : panel(_panel)
		{

		}

		void Render(IGRRenderContext& g)
		{

		}

		void Free() override
		{
			delete this;
		}

		IGRPanel& Panel()
		{
			return panel;
		}
	};
}

namespace Rococo::Gui
{
	IGRMainFrameSupervisor* CreateGRMainFrame(IGRPanel& panel)
	{
		return new ANON::GRMainFrame(panel);
	}
}