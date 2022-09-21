#include <rococo.gui.retained.h>
#include <rococo.maths.h>

using namespace Rococo;
using namespace Rococo::Gui;

namespace GRANON
{
	struct GRDivision : IGRWidgetDivision
	{
		IGRPanel& panel;

		GRDivision(IGRPanel& owningPanel) : panel(owningPanel)
		{
		}

		void Free() override
		{
			delete this;
		}

		void Layout(const GuiRect& panelDimensions) override
		{
			LayoutChildrenByAnchors(panel, panelDimensions);
		}

		EventRouting OnCursorClick(CursorEvent& ce) override
		{
			return EventRouting::NextHandler;
		}

		EventRouting OnCursorMove(CursorEvent& ce) override
		{
			return EventRouting::NextHandler;
		}

		IGRPanel& Panel() override
		{
			return panel;
		}

		void Render(IGRRenderContext& g) override
		{
			auto rect = panel.AbsRect();

			bool isHovered = g.IsHovered(panel);

			RGBAb backColour = panel.Root().Scheme().GetColour(isHovered ? ESchemeColourSurface::CONTAINER_BACKGROUND_HOVERED : ESchemeColourSurface::CONTAINER_BACKGROUND);
			g.DrawRect(rect, backColour);

			RGBAb edge1Colour = panel.Root().Scheme().GetColour(isHovered ? ESchemeColourSurface::CONTAINER_TOP_LEFT_HOVERED : ESchemeColourSurface::CONTAINER_TOP_LEFT);
			RGBAb edge2Colour = panel.Root().Scheme().GetColour(isHovered ? ESchemeColourSurface::CONTAINER_BOTTOM_RIGHT_HOVERED : ESchemeColourSurface::CONTAINER_BOTTOM_RIGHT);
			g.DrawRectEdge(rect, edge1Colour, edge2Colour);
		}

		EventRouting OnChildEvent(WidgetEvent& widgetEvent, IGRWidget& sourceWidget)
		{
			return EventRouting::NextHandler;
		}

		Vec2i EvaluateMinimalSpan() const override
		{
			return { 0,0 };
		}
	};

	struct GRDivFactory : IGRWidgetFactory
	{
		IGRWidget& CreateWidget(IGRPanel& panel)
		{
			return *new GRDivision(panel);
		}
	} s_DivFactory;
}

namespace Rococo::Gui
{
	ROCOCO_GUI_RETAINED_API IGRWidgetDivision& CreateDivision(IGRWidget& parent)
	{
		auto& gr = parent.Panel().Root().GR();
		auto& div = static_cast<IGRWidgetDivision&>(gr.AddWidget(parent.Panel(), GRANON::s_DivFactory));
		return div;
	}
}