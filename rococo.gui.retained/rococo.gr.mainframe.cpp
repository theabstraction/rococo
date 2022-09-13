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
		g.DrawRect(panel.ParentOffset(), panel.Span(), colour);
	}

	ROCOCO_GUI_RETAINED_API void DrawButton(IGRPanel& panel, bool hovered, bool focused, bool raised, IGRRenderContext& g)
	{
		ESchemeColourSurface surface;
		if (hovered)
		{
			surface = raised ? ESchemeColourSurface::BUTTON_RAISED_AND_HOVERED : ESchemeColourSurface::BUTTON_PRESSED_AND_HOVERED;
		}
		else
		{
			surface = raised ? ESchemeColourSurface::BUTTON_RAISED : ESchemeColourSurface::BUTTON_PRESSED;
		}

		RGBAb colour = panel.Root().Scheme().GetColour(surface);
		g.DrawRect(panel.ParentOffset(), panel.Span(), colour);

		ESchemeColourSurface topLeftEdge;
		ESchemeColourSurface bottomRightEdge;

		topLeftEdge = raised ? ESchemeColourSurface::BUTTON_EDGE_TOP_LEFT : ESchemeColourSurface::BUTTON_EDGE_TOP_LEFT_PRESSED;
		bottomRightEdge = raised ? ESchemeColourSurface::BUTTON_EDGE_BOTTOM_RIGHT : ESchemeColourSurface::BUTTON_EDGE_BOTTOM_RIGHT_PRESSED;

		RGBAb colour1 = panel.Root().Scheme().GetColour(topLeftEdge);
		RGBAb colour2 = panel.Root().Scheme().GetColour(bottomRightEdge);

		g.DrawRectEdge(panel.ParentOffset(), panel.Span(), colour1, colour2);
	}
}