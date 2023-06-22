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

		void PreRender(IGRPanel& panel, const GuiRect& absRect, IGRRenderContext& g) override
		{

		}

		void PostRender(IGRPanel& panel, const GuiRect& absRect, IGRRenderContext& g) override
		{
			GuiRect triangleRect = { absRect.left + 2, absRect.top + 2, absRect.right - 2, absRect.bottom - 2 };

			bool isHovered = g.IsHovered(panel);

			GRRenderState rs(false, isHovered, false);

			RGBAb triangleColour = panel.GetColour(EGRSchemeColourSurface::SCROLLER_TRIANGLE_NORMAL, rs);
			g.DrawDirectionArrow(triangleRect, triangleColour, orientation);
		}

		bool IsReplacementForWidgetRendering(IGRPanel& panel) const override
		{
			return false;
		}
	};

	struct GRVerticalScrollerWithButtons : IGRWidgetVerticalScrollerWithButtons, IGRWidget
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
			topButton = &CreateButton(*this);
			bottomButton = &CreateButton(*this);
			scroller = &CreateVerticalScroller(*this, events);

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

		void Layout(const GuiRect& panelDimensions) override
		{
			int32 width = Width(panelDimensions);
			topButton->Widget().Panel().SetParentOffset({ 0,0 });
			topButton->Widget().Panel().Resize({ width, width });

			scroller->Widget().Panel().SetParentOffset({ 0, width });
			scroller->Widget().Panel().Resize({ width, Height(panelDimensions) - 2 * width});

			bottomButton->Widget().Panel().SetParentOffset({ 0, Height(panelDimensions) - width });
			bottomButton->Widget().Panel().Resize({ width, width });

			topButton->Widget().Panel().InvalidateLayout(false);
			scroller->Widget().Panel().InvalidateLayout(false);
			bottomButton->Widget().Panel().InvalidateLayout(false);

			RGBAb backColour = panel.GetColour(EGRSchemeColourSurface::SCROLLER_BUTTON_BACKGROUND, GRGenerateIntensities());
			panel.Set(EGRSchemeColourSurface::BUTTON, backColour, GRGenerateIntensities());

			RGBAb tlEdgeColour = panel.GetColour(EGRSchemeColourSurface::SCROLLER_BUTTON_TOP_LEFT, GRGenerateIntensities());
			panel.Set(EGRSchemeColourSurface::BUTTON_EDGE_TOP_LEFT, tlEdgeColour, GRGenerateIntensities());

			RGBAb brEdgeColour = panel.GetColour(EGRSchemeColourSurface::SCROLLER_BUTTON_BOTTOM_RIGHT, GRGenerateIntensities());
			panel.Set(EGRSchemeColourSurface::BUTTON_EDGE_BOTTOM_RIGHT, brEdgeColour, GRGenerateIntensities());
		}

		EGREventRouting OnCursorClick(GRCursorEvent& ce) override
		{
			return EGREventRouting::NextHandler;
		}

		EGREventRouting OnCursorMove(GRCursorEvent& ce) override
		{
			return EGREventRouting::NextHandler;
		}

		IGRPanel& Panel() override
		{
			return panel;
		}

		void Render(IGRRenderContext& g) override
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

		EGREventRouting OnKeyEvent(GRKeyEvent& keyEvent) override
		{
			return EGREventRouting::NextHandler;
		}

		EGRQueryInterfaceResult QueryInterface(IGRBase** ppOutputArg, cstr interfaceId) override
		{
			return Gui::QueryForParticularInterface<IGRWidgetVerticalScrollerWithButtons>(this, ppOutputArg, interfaceId);
		}

		IGRWidget& Widget() override
		{
			return *this;
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