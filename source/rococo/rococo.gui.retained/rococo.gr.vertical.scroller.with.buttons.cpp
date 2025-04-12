#include <rococo.gui.retained.ex.h>
#include <rococo.maths.i32.h>
#include <rococo.maths.h>

using namespace Rococo;
using namespace Rococo::Gui;

namespace ANON
{
	struct ScrollerButtonRenderer : IGRPanelRenderer
	{
		Degrees orientation;

		void PreRender(IGRPanel& panel, const GuiRect& absRect, IGRRenderContext&) override
		{
			UNUSED(panel);
			UNUSED(absRect);
		}

		void PostRender(IGRPanel& panel, const GuiRect& absRect, IGRRenderContext& g) override
		{
			GuiRect triangleRect = { absRect.left + 3, absRect.top + 3, absRect.right - 3, absRect.bottom - 3 };

			bool isHovered = g.IsHovered(panel);

			GRRenderState rs(false, isHovered, false);

			RGBAb triangleColour = panel.GetColour(EGRSchemeColourSurface::SCROLLER_TRIANGLE_NORMAL, rs);
			g.DrawDirectionArrow(triangleRect, triangleColour, orientation);
		}

		bool IsReplacementForWidgetRendering(IGRPanel&) const override
		{
			return false;
		}
	};

	struct GRVerticalScrollerWithButtons : IGRWidgetVerticalScrollerWithButtons, IGRWidgetSupervisor, IGRWidgetLayout
	{
		IGRPanel& panel;
		IGRWidgetButton* topButton = nullptr;
		IGRWidgetButton* bottomButton = nullptr;
		IGRWidgetVerticalScroller* scroller = nullptr;
		IGRScrollerEvents& events;

		ScrollerButtonRenderer upRenderer;
		ScrollerButtonRenderer downRenderer;

		GRVerticalScrollerWithButtons(IGRPanel& owningPanel, IGRScrollerEvents& argEvents) : panel(owningPanel), events(argEvents)
		{
			upRenderer.orientation = 0_degrees;
			downRenderer.orientation = 180_degrees;
		}

		void PostConstruct()
		{
			panel.SetLayoutDirection(ELayoutDirection::None);
			topButton = &CreateButton(*this);
			bottomButton = &CreateButton(*this);
			scroller = &CreateVerticalScroller(*this, events);
			scroller->Widget().Panel().SetExpandToParentHorizontally();
			topButton->Widget().Panel().SetPanelRenderer(&upRenderer);
			topButton->SetEventPolicy(EGREventPolicy::NotifyAncestors);
			bottomButton->Widget().Panel().SetPanelRenderer(&downRenderer);
			bottomButton->SetEventPolicy(EGREventPolicy::NotifyAncestors);
		}

		void Free() override
		{
			delete this;
		}

		IGRWidgetButton& BottomButton() override
		{
			return *bottomButton;
		}

		IGRWidgetVerticalScroller& Scroller() override
		{
			return *scroller;
		}

		IGRWidgetButton& TopButton() override
		{
			return *topButton;
		}

		void LayoutBeforeFit() override
		{
		}

		void LayoutBeforeExpand() override
		{
			

		}

		void LayoutAfterExpand() override
		{
			int buttonSpan = panel.Span().x;
			topButton->Widget().Panel().SetConstantSpan(Vec2i{ buttonSpan, buttonSpan });
			scroller->Widget().Panel().SetConstantHeight(panel.Span().y - 2 * buttonSpan).SetParentOffset({ 0, buttonSpan });;
			bottomButton->Widget().Panel().SetConstantSpan(Vec2i{ buttonSpan, buttonSpan }).SetParentOffset({ 0, panel.Span().y - buttonSpan });

			CopyAllColours(panel, panel, EGRSchemeColourSurface::SCROLLER_BUTTON_BACKGROUND, EGRSchemeColourSurface::BUTTON);
			CopyAllColours(panel, panel, EGRSchemeColourSurface::SCROLLER_BUTTON_TOP_LEFT, EGRSchemeColourSurface::BUTTON_EDGE_TOP_LEFT);
			CopyAllColours(panel, panel, EGRSchemeColourSurface::SCROLLER_BUTTON_BOTTOM_RIGHT, EGRSchemeColourSurface::BUTTON_EDGE_BOTTOM_RIGHT);
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

		IGRPanel& Panel() override
		{
			return panel;
		}

		void Render(IGRRenderContext&) override
		{
		}

		EGREventRouting OnChildEvent(GRWidgetEvent& widgetEvent, IGRWidget& sourceWidget) override
		{
			if (widgetEvent.eventType == EGRWidgetEventType::BUTTON_CLICK)
			{
				if (&sourceWidget == &topButton->Widget())
				{
					events.OnScrollLines(-1, *this->scroller);
					// The top button was clicked
					return EGREventRouting::Terminate;
				}
				else if (&sourceWidget == &bottomButton->Widget())
				{
					events.OnScrollLines(1, *this->scroller);
					// The bottom button was clicked
					return EGREventRouting::Terminate;
				}
			}
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

		EGRQueryInterfaceResult QueryInterface(IGRBase** ppOutputArg, cstr interfaceId) override
		{
			auto result = Gui::QueryForParticularInterface<IGRWidgetLayout>(this, ppOutputArg, interfaceId);
			if (result == EGRQueryInterfaceResult::SUCCESS)
			{
				return result;
			}

			return Gui::QueryForParticularInterface<IGRWidgetVerticalScrollerWithButtons>(this, ppOutputArg, interfaceId);
		}

		IGRWidget& Widget() override
		{
			return *this;
		}

		cstr GetImplementationTypeName() const override
		{
			return "GRVerticalScrollerWithButtons";
		}
	};

	struct VerticalScrollerWithButtonsFactory : IGRWidgetFactory
	{
		IGRScrollerEvents& events;
		VerticalScrollerWithButtonsFactory(IGRScrollerEvents& argEvents): events(argEvents)
		{

		}

		IGRWidget& CreateWidget(IGRPanel& panel)
		{
			return *new GRVerticalScrollerWithButtons(panel, events);
		}
	};
}

namespace Rococo::Gui
{
	ROCOCO_GUI_RETAINED_API cstr IGRWidgetVerticalScrollerWithButtons::InterfaceId()
	{
		return "IGRWidgetVerticalScrollerWithButtons";
	}

	ROCOCO_GUI_RETAINED_API IGRWidgetVerticalScrollerWithButtons& CreateVerticalScrollerWithButtons(IGRWidget& parent, IGRScrollerEvents& events)
	{
		ANON::VerticalScrollerWithButtonsFactory factory(events);

		auto& gr = parent.Panel().Root().GR();
		auto* scroller = Cast<IGRWidgetVerticalScrollerWithButtons>(gr.AddWidget(parent.Panel(), factory));
		auto* scrollerClass = static_cast<ANON::GRVerticalScrollerWithButtons*>(scroller);
		scrollerClass->PostConstruct();
		return *scroller;
	}
}