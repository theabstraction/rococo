#include <rococo.gui.retained.ex.h>
#include <rococo.maths.i32.h>
#include <string>

using namespace Rococo;
using namespace Rococo::Gui;

namespace GRANON
{
	struct GRText : IGRWidgetText, IGRWidget
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

		void Layout(const GuiRect& panelDimensions) override
		{
		}

		EGREventRouting OnCursorClick(GRCursorEvent& ce) override
		{
			return EGREventRouting::NextHandler;
		}

		EGREventRouting OnCursorMove(GRCursorEvent& ce) override
		{
			return EGREventRouting::NextHandler;
		}

		void OnCursorEnter() override
		{

		}

		void OnCursorLeave() override
		{

		}

		EGREventRouting OnKeyEvent(GRKeyEvent& keyEvent) override
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
			g.DrawText(fontId, rect, rect, alignment, spacing, { text.c_str(), (int32)text.length() }, panel.GetColour(EGRSchemeColourSurface::TEXT, rs));
		}

		EGREventRouting OnChildEvent(GRWidgetEvent& widgetEvent, IGRWidget& sourceWidget)
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