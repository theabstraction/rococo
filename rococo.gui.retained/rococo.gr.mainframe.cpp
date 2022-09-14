#include <rococo.gui.retained.h>
#include <rococo.maths.h>

namespace ANON
{
	using namespace Rococo;
	using namespace Rococo::Gui;

	struct GRMainFrame: IGRMainFrameSupervisor
	{
		IGRPanel& panel;

		GRMainFrame(IGRPanel& _panel) : panel(_panel)
		{
			
		}

		void Render(IGRRenderContext& g)
		{
			DrawPanelBackground(panel, g);
		}

		void Free() override
		{
			delete this;
		}

		void Layout(const GuiRect& screenDimensions) override
		{
			panel.SetParentOffset(TopLeft(screenDimensions));
			panel.Resize(Span(screenDimensions));
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

	ROCOCO_GUI_RETAINED_API void DrawPanelBackground(IGRPanel& panel, IGRRenderContext& g)
	{
		RGBAb colour = panel.Root().Scheme().GetColour(ESchemeColourSurface::BACKGROUND);
		g.DrawRect(panel.AbsRect(), colour);
	}
}