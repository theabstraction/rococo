#include <rococo.gui.retained.ex.h>
#include <rococo.maths.i32.h>
#include <string>

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

		EGREventRouting OnCursorClick(GRCursorEvent& ce) override
		{
			UNUSED(ce);
			return EGREventRouting::NextHandler;
		}

		EGREventRouting OnCursorMove(GRCursorEvent& ce) override
		{
			UNUSED(ce);
			return EGREventRouting::NextHandler;
		}

		void OnCursorEnter() override
		{

		}

		void OnCursorLeave() override
		{

		}

		IGRPanel& Panel() override
		{
			return panel;
		}

		void Render(IGRRenderContext& g) override
		{
			auto rect = panel.AbsRect();

			GRRenderState rs(false, g.IsHovered(panel), false);

			RGBAb backColour = panel.GetColour(EGRSchemeColourSurface::CONTAINER_BACKGROUND, rs);
			g.DrawRect(rect, backColour);

			RGBAb edge1Colour = panel.GetColour(EGRSchemeColourSurface::CONTAINER_TOP_LEFT, rs);
			RGBAb edge2Colour = panel.GetColour(EGRSchemeColourSurface::CONTAINER_BOTTOM_RIGHT, rs);
			g.DrawRectEdge(rect, edge1Colour, edge2Colour);
		}

		EGREventRouting OnChildEvent(GRWidgetEvent&, IGRWidget&)
		{
			return EGREventRouting::NextHandler;
		}

		EGREventRouting OnKeyEvent(GRKeyEvent&) override
		{
			return EGREventRouting::NextHandler;
		}

		EGRQueryInterfaceResult QueryInterface(IGRBase** ppOutputArg, cstr interfaceId) override
		{
			return QueryForParticularInterface<IGRWidgetDivision>(this, ppOutputArg, interfaceId);
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
	ROCOCO_GUI_RETAINED_API cstr IGRWidgetDivision::InterfaceId()
	{
		return "IGRWidgetDivision";
	}

	ROCOCO_GUI_RETAINED_API IGRWidgetDivision& CreateDivision(IGRWidget& parent)
	{
		auto& gr = parent.Panel().Root().GR();
		auto& div = static_cast<IGRWidgetDivision&>(gr.AddWidget(parent.Panel(), GRANON::s_DivFactory));
		return div;
	}
}