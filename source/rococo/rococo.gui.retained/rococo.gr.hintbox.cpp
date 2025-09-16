#include <rococo.gui.retained.ex.h>
#include <rococo.maths.i32.h>
#include <rococo.strings.h>

using namespace Rococo;
using namespace Rococo::Gui;

namespace GRANON
{
	struct GRHintBox : IGRWidgetDivisionWithText, IGRWidgetSupervisor
	{
		IGRPanel& panel;

		float transparency = 1.0f;
		GRAlignmentFlags alignment;

		GRHintBox(IGRPanel& owningPanel) : panel(owningPanel)
		{
			alignment.Add(EGRAlignment::HCentre).Add(EGRAlignment::VCentre);
		}

		virtual ~GRHintBox()
		{

		}

		void OnTick(float dt) override
		{
			UNUSED(dt);
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

		void DrawTextWithShadow(IGRRenderContext& g, const GuiRect& targetRect, const fstring& text)
		{
			GuiRect shadowRect = targetRect;
			shadowRect.left += 1;
			shadowRect.top += 1;
			shadowRect.right += 1;
			shadowRect.bottom += 1;
			g.DrawParagraph(fontId, shadowRect, alignment, spacing, text, RGBAb(0, 0, 0, 255));
			g.DrawParagraph(fontId, targetRect, alignment, spacing, text, RGBAb(255, 255, 255, 255));
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

			GRWidgetEvent evGetHoverHint;
			evGetHoverHint.clickPosition = { 0,0 };
			evGetHoverHint.eventType = EGRWidgetEventType::GET_HINT_HOVER;
			evGetHoverHint.iMetaData = 0;
			evGetHoverHint.isCppOnly = true;
			evGetHoverHint.panelId = panel.Id();
			evGetHoverHint.sMetaData = nullptr;
			static_cast<IGRPanelSupervisor&>(panel).RouteToParent(evGetHoverHint);
			cstr hint = evGetHoverHint.sMetaData;

			if (hint != nullptr && *hint != 0)
			{
				DrawTextWithShadow(g, panel.AbsRect(), to_fstring(hint));
			}
			else
			{
				auto* focusWidget = panel.Root().GR().FindFocusWidget();
				if (focusWidget)
				{
					hint = focusWidget->Panel().Hint();
					if (*hint != 0)
					{
						DrawTextWithShadow(g, panel.AbsRect(), to_fstring(hint));
					}
				}
			}
		}

		GRFontId fontId = GRFontId::NONE;
		Vec2i spacing{ 0,0 };

		void SetAlignment(GRAlignmentFlags flags) override
		{
			alignment = flags;
		}

		void SetFont(GRFontId fontId) override
		{
			this->fontId = fontId;
		}

		void SetSpacing(Vec2i spacing) override
		{
			this->spacing = spacing;
		}

		EGREventRouting OnChildEvent(GRWidgetEvent&, IGRWidget&) override
		{
			return EGREventRouting::NextHandler;
		}

		EGREventRouting OnKeyEvent(GRKeyEvent&) override
		{
			return EGREventRouting::NextHandler;
		}

		EGRQueryInterfaceResult QueryInterface(IGRBase** ppOutputArg, cstr interfaceId) override
		{
			return QueryForParticularInterface<IGRWidgetDivisionWithText>(this, ppOutputArg, interfaceId);
		}

		cstr GetImplementationTypeName() const override
		{
			return "GRHintBox";
		}
	};

	struct GRHintFactory : IGRWidgetFactory
	{
		IGRWidget& CreateWidget(IGRPanel& panel)
		{
			return *new GRHintBox(panel);
		}
	} s_HintFactory;
}

namespace Rococo::Gui
{
	ROCOCO_GUI_RETAINED_API IGRWidgetDivisionWithText& CreateHintBox(IGRWidget& parent)
	{
		auto& gr = parent.Panel().Root().GR();
		auto& hintBox = static_cast<GRANON::GRHintBox&>(gr.AddWidget(parent.Panel(), GRANON::s_HintFactory));
		return hintBox;
	}
}