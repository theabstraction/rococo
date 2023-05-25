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

			RGBAb triangleColour = panel.GetColour(isHovered ? ESchemeColourSurface::SCROLLER_TRIANGLE_HOVERED : ESchemeColourSurface::SCROLLER_TRIANGLE_NORMAL);
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
		IScrollerEvents* events = nullptr; // Always is set to non-null by the factory function at the bottom of the file

		ScrollerButtonRenderer upRenderer;
		ScrollerButtonRenderer downRenderer;

		GRVerticalScrollerWithButtons(IGRPanel& owningPanel) : panel(owningPanel)
		{
			upRenderer.orientation = 0_degrees;
			downRenderer.orientation = 180_degrees;
		}

		void PostConstruct(IScrollerEvents& events)
		{
			topButton = &CreateButton(*this);
			bottomButton = &CreateButton(*this);
			scroller = &CreateVerticalScroller(*this, events);

			topButton->Widget().Panel().SetPanelRenderer(&upRenderer);
			bottomButton->Widget().Panel().SetPanelRenderer(&downRenderer);
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
		}


		EventRouting OnCursorClick(CursorEvent& ce) override
		{
			return EventRouting::NextHandler;
		}

		EventRouting OnCursorMove(CursorEvent& ce) override
		{
			return EventRouting::NextHandler;
		}

		IGRPanel& Panel() override
		{
			return panel;
		}

		
		void Render(IGRRenderContext& g) override
		{
			RGBAb backColour = panel.GetColour(ESchemeColourSurface::SCROLLER_BUTTON_BACKGROUND);
			panel.Set(ESchemeColourSurface::BUTTON_RAISED, backColour);

			RGBAb backColourHi = panel.GetColour(ESchemeColourSurface::SCROLLER_BUTTON_BACKGROUND_HOVERED);
			panel.Set(ESchemeColourSurface::BUTTON_RAISED_AND_HOVERED, backColourHi);

			RGBAb tlEdgeColour = panel.GetColour(ESchemeColourSurface::SCROLLER_BUTTON_TOP_LEFT);
			panel.Set(ESchemeColourSurface::BUTTON_EDGE_TOP_LEFT, tlEdgeColour);

			RGBAb brEdgeColour = panel.GetColour(ESchemeColourSurface::SCROLLER_BUTTON_BOTTOM_RIGHT);
			panel.Set(ESchemeColourSurface::BUTTON_EDGE_BOTTOM_RIGHT, brEdgeColour);
		}

		EventRouting OnChildEvent(WidgetEvent& widgetEvent, IGRWidget& sourceWidget) override
		{
			return EventRouting::NextHandler;
		}

		void OnCursorEnter() override
		{
		}

		void OnCursorLeave() override
		{
		}

		EventRouting OnKeyEvent(KeyEvent& keyEvent) override
		{
			return EventRouting::NextHandler;
		}

		Vec2i EvaluateMinimalSpan() const override
		{
			return { 0,0 };
		}

		EQueryInterfaceResult QueryInterface(IGRBase** ppOutputArg, cstr interfaceId) override
		{
			return Gui::QueryForParticularInterface<IGRWidgetVerticalScrollerWithButtons>(this, ppOutputArg, interfaceId);
		}

		IGRWidget& Widget() override
		{
			return *this;
		}
	};

	struct  : IGRWidgetFactory
	{
		IGRWidget& CreateWidget(IGRPanel& panel)
		{
			return *new GRVerticalScrollerWithButtons(panel);
		}
	} s_VerticalScrollerWithButtonsFactory;
}

namespace Rococo::Gui
{
	ROCOCO_GUI_RETAINED_API cstr IGRWidgetVerticalScrollerWithButtons::InterfaceId()
	{
		return "IGRWidgetVerticalScrollerWithButtons";
	}

	ROCOCO_GUI_RETAINED_API IGRWidgetVerticalScrollerWithButtons& CreateVerticalScrollerWithButtons(IGRWidget& parent, IScrollerEvents& events)
	{
		auto& gr = parent.Panel().Root().GR();
		auto* scroller = Cast<IGRWidgetVerticalScrollerWithButtons>(gr.AddWidget(parent.Panel(), ANON::s_VerticalScrollerWithButtonsFactory));
		auto* scrollerClass = static_cast<ANON::GRVerticalScrollerWithButtons*>(scroller);
		scrollerClass->PostConstruct(events);
		return *scroller;
	}
}