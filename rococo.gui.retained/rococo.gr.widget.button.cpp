#include <rococo.gui.retained.h>
#include <rococo.maths.h>

using namespace Rococo;
using namespace Rococo::Gui;

namespace ANON
{
	struct Button : IGRWidgetButton
	{
		IGRPanel& panel;
		bool isRaised = true;

		Button(IGRPanel& owningPanel) : panel(owningPanel)
		{

		}

		void Free() override
		{
			delete this;
		}

		void Layout(const GuiRect& parentDimensions) override
		{

		}

		EventRouting OnCursorClick(CursorEvent& ce) override
		{
			if (ce.click.LeftButtonDown)
			{
				isRaised = false;
				return EventRouting::Terminate;
			}
			else if (ce.click.LeftButtonUp)
			{
				isRaised = true;
				return EventRouting::Terminate;
			}

			return EventRouting::NextChild;
		}

		EventRouting OnCursorMove(CursorEvent& ce) override
		{
			return EventRouting::NextChild;
		}

		IGRPanel& Panel() override
		{
			return panel;
		}

		void Render(IGRRenderContext& g) override
		{
			DrawButton(panel, false, isRaised, g);
		}
	};

	struct ButtonFactory : IGRWidgetFactory
	{
		IGRWidget& CreateWidget(IGRPanel& panel)
		{
			return *new Button(panel);
		}
	} s_ButtonFactory;
}

namespace Rococo::Gui
{
	ROCOCO_GUI_RETAINED_API IGRWidgetButton& CreateButton(IGRWidget& parent)
	{
		auto& gr = parent.Panel().Root().GR();
		return static_cast<IGRWidgetButton&>(gr.AddWidget(parent.Panel(), ANON::s_ButtonFactory));
	}

	ROCOCO_GUI_RETAINED_API void DrawButton(IGRPanel& panel, bool focused, bool raised, IGRRenderContext& g)
	{
		bool hovered = g.IsHovered(panel);

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
		g.DrawRect(panel.AbsRect(), colour);

		ESchemeColourSurface topLeftEdge;
		ESchemeColourSurface bottomRightEdge;

		topLeftEdge = raised ? ESchemeColourSurface::BUTTON_EDGE_TOP_LEFT : ESchemeColourSurface::BUTTON_EDGE_TOP_LEFT_PRESSED;
		bottomRightEdge = raised ? ESchemeColourSurface::BUTTON_EDGE_BOTTOM_RIGHT : ESchemeColourSurface::BUTTON_EDGE_BOTTOM_RIGHT_PRESSED;

		RGBAb colour1 = panel.Root().Scheme().GetColour(topLeftEdge);
		RGBAb colour2 = panel.Root().Scheme().GetColour(bottomRightEdge);

		g.DrawRectEdge(panel.AbsRect(), colour1, colour2);
	}
}