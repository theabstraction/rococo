#include <rococo.gui.retained.ex.h>
#include <rococo.maths.i32.h>
#include <rococo.strings.h>

using namespace Rococo;
using namespace Rococo::Gui;
using namespace Rococo::Strings;

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

		void Render(IGRRenderContext& g) override
		{
			DrawPanelBackgroundEx(
				panel,
				g, 
				EGRSchemeColourSurface::CONTAINER_BACKGROUND,
				EGRSchemeColourSurface::CONTAINER_TOP_LEFT,
				EGRSchemeColourSurface::CONTAINER_BOTTOM_RIGHT,
				transparency
			);
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

	RGBAb Modulate(RGBAb colour, float alphaScale)
	{
		float alpha = clamp(alphaScale * (float)colour.alpha, 0.0f, 255.0f);
		return RGBAb(colour.red, colour.green, colour.blue, (uint8)alpha);
	}

	ROCOCO_GUI_RETAINED_API void DrawPanelBackgroundEx(
		IGRPanel& panel, 
		IGRRenderContext& g,
		EGRSchemeColourSurface back, 
		EGRSchemeColourSurface leftEdge,
		EGRSchemeColourSurface rightEdge, 
		float alphaScale,
		bool isRaised,
		bool isFocused)
	{
		auto rect = panel.AbsRect();

		GRWidgetRenderState rs(!isRaised, g.IsHovered(panel), isFocused);

		RGBAb backColour = panel.GetColour(back, rs);
		g.DrawRect(rect, Modulate(backColour, alphaScale), panel.RectStyle(), panel.CornerRadius());

		RGBAb edge1Colour = panel.GetColour(leftEdge, rs);
		RGBAb edge2Colour = panel.GetColour(rightEdge, rs);
		g.DrawRectEdge(rect, Modulate(edge1Colour, alphaScale), Modulate(edge2Colour, alphaScale), panel.RectStyle(), panel.CornerRadius());
	}
}