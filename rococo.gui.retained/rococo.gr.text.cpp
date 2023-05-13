#include <rococo.gui.retained.h>
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
		}

		void Free() override
		{
			delete this;
		}

		void Layout(const GuiRect& panelDimensions) override
		{
		}

		EventRouting OnCursorClick(CursorEvent& ce) override
		{
			return EventRouting::NextHandler;
		}

		EventRouting OnCursorMove(CursorEvent& ce) override
		{
			return EventRouting::NextHandler;
		}

		EventRouting OnKeyEvent(KeyEvent& keyEvent) override
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

			g.DrawText(fontId, rect, alignment, spacing, { text.c_str(), (int32)text.length() }, panel.GetColour(isHovered ? ESchemeColourSurface::TEXT_HOVERED : ESchemeColourSurface::TEXT));
		}

		EventRouting OnChildEvent(WidgetEvent& widgetEvent, IGRWidget& sourceWidget)
		{
			return EventRouting::NextHandler;
		}

		Vec2i EvaluateMinimalSpan() const override
		{
			return { 10,10 };
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

		EQueryInterfaceResult QueryInterface(IGRBase** ppOutputArg, cstr interfaceId) override
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