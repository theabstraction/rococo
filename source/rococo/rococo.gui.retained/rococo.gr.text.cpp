#include <rococo.gui.retained.ex.h>
#include <rococo.maths.i32.h>
#include <string>

using namespace Rococo;
using namespace Rococo::Gui;

namespace GRANON
{
	struct GRText : IGRWidgetText, IGRWidgetSupervisor, IGRWidgetLayout
	{
		IGRPanel& panel;
		std::string text;
		GRFontId fontId = GRFontId::MENU_FONT;
		GRAlignmentFlags alignment;
		Vec2i spacing { 0,0 };
		EGRSchemeColourSurface labelSurface = EGRSchemeColourSurface::TEXT;
		EGRSchemeColourSurface backSurface = EGRSchemeColourSurface::LABEL_BACKGROUND;

		enum { MAX_LENGTH = 128 };

		GRText(IGRPanel& owningPanel) : panel(owningPanel)
		{
			owningPanel.SetMinimalSpan({ 10,10 });
		}

		void Free() override
		{
			delete this;
		}

		bool expandToFitTextH = false;
		bool expandToFitTextV = false;

		void FitTextH() override
		{
			expandToFitTextH = true;
		}

		void FitTextV() override
		{
			expandToFitTextV = true;
		}

		void LayoutBeforeFit() override
		{
			Vec2i fit = ComputeFit();
			fit.x += panel.Padding().left + panel.Padding().right;
			fit.y += panel.Padding().top + panel.Padding().bottom;
			
			if (expandToFitTextV)
			{
				panel.SetConstantHeight(fit.y);
			}

			if (expandToFitTextH)
			{
				panel.SetConstantHeight(fit.x);
			}
		}

		void LayoutBeforeExpand() override
		{

		}

		void LayoutAfterExpand() override
		{

		}

		Vec2i ComputeFit() const
		{
			return panel.Root().Custodian().EvaluateMinimalSpan(fontId, fstring{ text.c_str(), (int32)text.length() });
		}


		int TextWidth() const override
		{
			return ComputeFit().x;
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

			RGBAb backColour = panel.GetColour(backSurface, rs);
			g.DrawRect(rect, backColour);

			RGBAb edge1Colour = panel.GetColour(EGRSchemeColourSurface::CONTAINER_TOP_LEFT, rs);
			RGBAb edge2Colour = panel.GetColour(EGRSchemeColourSurface::CONTAINER_BOTTOM_RIGHT, rs);
			g.DrawRectEdge(rect, edge1Colour, edge2Colour);

			g.DrawText(fontId, rect, alignment, spacing, { text.c_str(), (int32)text.length() }, panel.GetColour(labelSurface, rs));
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

		IGRWidgetText& SetTextColourSurface(EGRSchemeColourSurface surface) override
		{
			labelSurface = surface;
			return *this;
		}

		IGRWidgetText& SetBackColourSurface(EGRSchemeColourSurface surface) override
		{
			backSurface = surface;
			return *this;
		}

		EGRQueryInterfaceResult QueryInterface(IGRBase** ppOutputArg, cstr interfaceId) override
		{
			if (Gui::QueryForParticularInterface<IGRWidgetLayout>(this, ppOutputArg, interfaceId) == EGRQueryInterfaceResult::SUCCESS)
			{
				return  EGRQueryInterfaceResult::SUCCESS;
			}
			
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