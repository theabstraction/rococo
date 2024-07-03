#include <rococo.gui.retained.ex.h>
#include <rococo.maths.i32.h>
#include <string>

using namespace Rococo;
using namespace Rococo::Gui;

namespace GRANON
{
	struct GRText : IGRWidgetText, IGRWidgetSupervisor
	{
		IGRPanel& panel;
		std::string text;
		GRFontId fontId = GRFontId::MENU_FONT;
		GRAlignmentFlags alignment;
		Vec2i spacing { 0,0 };

		enum { MAX_LENGTH = 128 };

		GRText(IGRPanel& owningPanel) : panel(owningPanel)
		{
			owningPanel.SetMinimalSpan({ 10,10 });
		}

		void Free() override
		{
			delete this;
		}

		int GetTextWidth() const override
		{
			return panel.Root().Custodian().EvaluateMinimalSpan(fontId, fstring { text.c_str(), (int32) text.length() }).x;
		}

		void Layout(const GuiRect& panelDimensions) override
		{
			UNUSED(panelDimensions);
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

		EGREventRouting OnKeyEvent(GRKeyEvent&) override
		{
			return EGREventRouting::NextHandler;
		}

		IGRPanel& Panel() override
		{
			return panel;
		}

		void Render(IGRRenderContext& g) override
		{
			auto rect = panel.AbsRect();
			bool isHovered = g.IsHovered(panel);

			GRRenderState rs(false, isHovered, false);

			RGBAb backColour = panel.GetColour(EGRSchemeColourSurface::LABEL_BACKGROUND, rs);
			g.DrawRect(rect, backColour);

			RGBAb edge1Colour = panel.GetColour(EGRSchemeColourSurface::CONTAINER_TOP_LEFT, rs);
			RGBAb edge2Colour = panel.GetColour(EGRSchemeColourSurface::CONTAINER_BOTTOM_RIGHT, rs);
			g.DrawRectEdge(rect, edge1Colour, edge2Colour);

			g.DrawText(fontId, rect, rect, alignment, spacing, { text.c_str(), (int32)text.length() }, panel.GetColour(EGRSchemeColourSurface::TEXT, rs));
		}

		EGREventRouting OnChildEvent(GRWidgetEvent&, IGRWidget&)
		{
			return EGREventRouting::NextHandler;
		}

		IGRWidgetText& SetAlignment(GRAlignmentFlags alignment, Vec2i spacing) override
		{
			this->alignment = alignment;
			this->spacing = spacing;
			return *this;
		}

		IGRWidgetText& SetFont(GRFontId fontId) override
		{
			this->fontId = fontId;
			return *this;
		}

		IGRWidgetText& SetText(cstr text) override
		{
			if (strlen(text) > MAX_LENGTH)
			{
				this->text = std::string(text, MAX_LENGTH);
			}
			else
			{
				this->text = text;
			}
			return *this;
		}

		EGRQueryInterfaceResult QueryInterface(IGRBase** ppOutputArg, cstr interfaceId) override
		{
			return Gui::QueryForParticularInterface<IGRWidgetText>(this, ppOutputArg, interfaceId);
		}

		IGRWidget& Widget()
		{
			return *this;
		}

		cstr GetImplementationTypeName() const override
		{
			return "GRText";
		}
	};

	struct GRTextFactory : Rococo::Gui::IGRWidgetFactory
	{
		IGRWidget& CreateWidget(IGRPanel& panel)
		{
			return *new GRText(panel);
		}
	} s_TextFactory;
}

namespace Rococo::Gui
{
	ROCOCO_GUI_RETAINED_API cstr IGRWidgetText::InterfaceId()
	{
		return "IGRWidgetText";
	}

	ROCOCO_GUI_RETAINED_API IGRWidgetText& CreateText(IGRWidget& parent)
	{
		auto& gr = parent.Panel().Root().GR();
		auto* t = Cast<IGRWidgetText>(gr.AddWidget(parent.Panel(), GRANON::s_TextFactory));
		return *t;
	}
}