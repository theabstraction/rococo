#include <rococo.gui.retained.ex.h>
#include <rococo.maths.i32.h>
#include <string>

using namespace Rococo;
using namespace Rococo::Gui;

namespace GRANON
{
	struct GRDivision : IGRWidgetDivision, IGRWidgetSupervisor
	{
		IGRPanel& panel;

		float transparency = 1.0f;

		GRDivision(IGRPanel& owningPanel) : panel(owningPanel)
		{
		}

		virtual ~GRDivision()
		{

		}

		IGRWidget& Widget() override
		{
			return *this;
		}

		void SetTransparency(float f) override
		{
			transparency = clamp(f, 0.0f, 1.0f);
		}

		IGRPanel& Panel() override
		{
			return panel;
		}

		void Free() override
		{
			delete this;
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

		RGBAb Modulate(RGBAb colour)
		{
			float alpha = transparency * (float)colour.alpha;
			return RGBAb(colour.red, colour.green, colour.blue, (uint8) alpha);
		}

		void Render(IGRRenderContext& g) override
		{
			auto rect = panel.AbsRect();

			GRRenderState rs(false, g.IsHovered(panel), false);

			RGBAb backColour = panel.GetColour(EGRSchemeColourSurface::CONTAINER_BACKGROUND, rs);
			g.DrawRect(rect, Modulate(backColour), panel.RectStyle(), panel.CornerRadius());

			RGBAb edge1Colour = panel.GetColour(EGRSchemeColourSurface::CONTAINER_TOP_LEFT, rs);
			RGBAb edge2Colour = panel.GetColour(EGRSchemeColourSurface::CONTAINER_BOTTOM_RIGHT, rs);
			g.DrawRectEdge(rect, Modulate(edge1Colour), Modulate(edge2Colour));
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

		cstr GetImplementationTypeName() const override
		{
			return "GRDivision";
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
		auto& div = static_cast<GRANON::GRDivision&>(gr.AddWidget(parent.Panel(), GRANON::s_DivFactory));
		return div;
	}
}