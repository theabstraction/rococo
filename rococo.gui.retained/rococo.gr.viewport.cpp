#include <rococo.gui.retained.ex.h>
#include <rococo.maths.i32.h>

using namespace Rococo;
using namespace Rococo::Gui;

namespace ANON
{
	struct GRViewportWidget : IGRWidgetViewport, IGRWidget, IScrollerEvents
	{
		IGRPanel& panel;
		IGRWidgetDivision* clipArea = nullptr; // Represents the rectangle to the left of the scroller
		IGRWidgetDivision* clientOffsetArea = nullptr; // Represents the scrolled data inside the clipArea
		IGRWidgetVerticalScrollerWithButtons* vscroller = nullptr;

		GRViewportWidget(IGRPanel& owningPanel) : panel(owningPanel)
		{
		}

		void PostConstruct()
		{
			clipArea = &CreateDivision(*this);
			clientOffsetArea = &CreateDivision(*clipArea);
			clientOffsetArea->Panel().PreventInvalidationFromChildren();
			vscroller = &CreateVerticalScrollerWithButtons(*this, *this);
		}

		void Free() override
		{
			delete this;
		}

		void Layout(const GuiRect& panelDimensions) override
		{
			enum { scrollbarWidth = 16 };

			clipArea->Panel().Resize({ Width(panelDimensions) - scrollbarWidth, Height(panelDimensions)});
			clipArea->Panel().SetParentOffset({ 0,0 });
			clipArea->Panel().InvalidateLayout(false);

			ScrollerMetrics m;
			vscroller->Scroller().GetMetrics(m);

			clientOffsetArea->Panel().Resize({ Width(panelDimensions) - scrollbarWidth, Height(panelDimensions) });
			clientOffsetArea->Panel().SetParentOffset({ 0, - m.PixelPosition });
			clientOffsetArea->Panel().InvalidateLayout(false);

			vscroller->Widget().Panel().Resize({ scrollbarWidth, Height(panelDimensions) - 1 });
			vscroller->Widget().Panel().SetParentOffset({ Width(panelDimensions) - scrollbarWidth, 0 });
			vscroller->Widget().Panel().InvalidateLayout(false);

			vscroller->Scroller().SetSliderHeight(128);
		}

		void OnScrollerNewPositionCalculated(int32 newPosition, IGRWidgetScroller& scroller) override
		{
			clipArea->Panel().InvalidateLayout(false);
			clientOffsetArea->Panel().SetParentOffset({ 0, -newPosition });
			clientOffsetArea->Panel().InvalidateLayout(false);
			InvalidateLayoutForAllDescendants(clientOffsetArea->Panel());
		}

		EventRouting OnCursorClick(CursorEvent& ce) override
		{
			return EventRouting::NextHandler;
		}

		EventRouting OnCursorMove(CursorEvent& ce) override
		{
			return EventRouting::NextHandler;
		}

		void OnCursorEnter() override
		{

		}

		void OnCursorLeave() override
		{

		}

		IGRPanel& Panel() override
		{
			return panel;
		}

		IGRWidgetDivision& ClientArea() override
		{
			return *clientOffsetArea;
		}

		IGRWidgetVerticalScrollerWithButtons& VScroller() override
		{
			return *vscroller;
		}

		void Render(IGRRenderContext& g) override
		{
			ScrollerMetrics metrics;
			vscroller->Scroller().GetMetrics(metrics);
			DrawPanelBackground(panel, g);
		}

		EventRouting OnChildEvent(WidgetEvent& widgetEvent, IGRWidget& sourceWidget) override
		{
			return EventRouting::NextHandler;
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
			return Gui::QueryForParticularInterface<IGRWidgetViewport>(this, ppOutputArg, interfaceId);
		}

		IGRWidget& Widget() override
		{
			return *this;
		}
	};

	struct : IGRWidgetFactory
	{
		IGRWidget& CreateWidget(IGRPanel& panel)
		{
			auto* instance = new GRViewportWidget(panel);
			instance->PostConstruct();
			return *instance;
		}
	} s_ViewportWidgetFactory;
}

namespace Rococo::Gui
{
	ROCOCO_GUI_RETAINED_API cstr IGRWidgetViewport::InterfaceId()
	{
		return "IGRViewportWidget";
	}

	ROCOCO_GUI_RETAINED_API IGRWidgetViewport& CreateViewportWidget(IGRWidget& parent)
	{
		auto& gr = parent.Panel().Root().GR();
		auto* scroller = Cast<IGRWidgetViewport>(gr.AddWidget(parent.Panel(), ANON::s_ViewportWidgetFactory));
		return *scroller;
	}
}