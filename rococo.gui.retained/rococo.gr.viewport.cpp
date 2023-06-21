#include <rococo.gui.retained.ex.h>
#include <rococo.maths.i32.h>

using namespace Rococo;
using namespace Rococo::Gui;

namespace ANON
{
	struct GRViewportWidget : IGRWidgetViewport, IGRWidget, IGRScrollerEvents
	{
		IGRPanel& panel;
		IGRWidgetDivision* clipArea = nullptr; // Represents the rectangle to the left of the scroller
		IGRWidgetDivision* clientOffsetArea = nullptr; // Represents the scrolled data inside the clipArea
		IGRWidgetVerticalScrollerWithButtons* vscroller = nullptr;

		GRViewportWidget(IGRPanel& owningPanel) : panel(owningPanel)
		{
			owningPanel.SetMinimalSpan({ 10, 10 });
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

			Vec2i clipSpan { Width(panelDimensions) - scrollbarWidth, Height(panelDimensions) };
			clipArea->Panel().Resize(clipSpan);
			clipArea->Panel().SetParentOffset({ 0,0 });
			clipArea->Panel().InvalidateLayout(false);

			Vec2i clientOffsetSpan{ clipSpan.x, max(lastKnownDomainHeight, clipSpan.y) };

			int parentOffset = 0;

			ScrollerMetrics m = vscroller->Scroller().GetMetrics();
			if (m.PixelRange > 0)
			{
				double cursor = clamp((double)m.PixelPosition / (double)m.PixelRange, 0.0, 1.0);
				parentOffset = (int)(cursor * (lastKnownDomainHeight - m.SliderZoneSpan));
			}

			clientOffsetArea->Panel().Resize(clientOffsetSpan);
			clientOffsetArea->Panel().SetParentOffset({ 0, -parentOffset });
			clientOffsetArea->Panel().InvalidateLayout(false);
			InvalidateLayoutForAllDescendants(clientOffsetArea->Panel());

			vscroller->Widget().Panel().Resize({ scrollbarWidth, Height(panelDimensions) - 2 });
			vscroller->Widget().Panel().SetParentOffset({ Width(panelDimensions) - scrollbarWidth, 1 });
			vscroller->Widget().Panel().InvalidateLayout(false);
		}

		GRSliderSpec OnCalculateSliderRect(int32 scrollerSpan, IGRWidgetScroller& scroller) override
		{
			double sliderSpanToScrollerSpan = scrollerSpan / (double)lastKnownDomainHeight;

			GRSliderSpec spec;
			spec.sliderSpanInPixels = (int) (sliderSpanToScrollerSpan * scrollerSpan);
			return spec;
		}

		// Check the interface method IGRWidgetViewport::SetMovePageScale(...) for documentation on this variable
		double pageDeltaScale = 0.75;

		// Check the interface method IGRWidgetViewport::SetLineDeltaPixels(...) for documentation on this variable
		int lineDeltaPixels = 10;

		void SetLineDeltaPixels(int lineDeltaPixels) override
		{
			this->lineDeltaPixels = clamp(lineDeltaPixels, 1, 1000'000);
		}

		void SetMovePageScale(double scaleFactor) override
		{
			pageDeltaScale = clamp(scaleFactor, 0.0, 2.0);
		}

		void OnMoveLine(int delta, IGRWidgetScroller& scroller)
		{
			if (lineDeltaPixels == 0) return;

			ScrollerMetrics m = scroller.GetMetrics();
			if (m.PixelRange > 0 && lastKnownDomainHeight > m.SliderZoneSpan)
			{
				double scale = (lastKnownDomainHeight - m.SliderZoneSpan) / (double)m.PixelRange;

				int32 deltaPixels = delta * lineDeltaPixels;

				int newPosition = deltaPixels / scale;

				if (newPosition == 0)
				{
					// Don't let the scaling completely eliminate the change, we must always have some scrolling
					newPosition = (delta > 0) ? 1 : -1;
				}

				OnScrollerNewPositionCalculated(newPosition, scroller);

				scroller.SetSliderPosition(clamp(newPosition + m.PixelPosition, 0, m.PixelRange));
			}
		}

		void OnMovePage(int delta, IGRWidgetScroller& scroller) override
		{
			ScrollerMetrics m = scroller.GetMetrics();
			if (m.PixelRange > 0 && lastKnownDomainHeight > m.SliderZoneSpan)
			{
				double scale = (lastKnownDomainHeight - m.SliderZoneSpan) / (double)m.PixelRange;

				int32 deltaPixels = delta * clipArea->Panel().Span().y;

				int newPosition = pageDeltaScale * deltaPixels / scale;

				OnScrollerNewPositionCalculated(newPosition, scroller);

				scroller.SetSliderPosition(clamp(newPosition + m.PixelPosition, 0, m.PixelRange));
			}
		}

		void OnScrollerNewPositionCalculated(int32 newPosition, IGRWidgetScroller& scroller) override
		{
			int parentOffset = 0;

			ScrollerMetrics m = scroller.GetMetrics();
			if (m.PixelRange > 0 && lastKnownDomainHeight > m.SliderZoneSpan)
			{
				double cursor = clamp((double)(m.PixelPosition + newPosition) / (double)m.PixelRange, 0.0, 1.0);
				parentOffset = (int)(cursor * (lastKnownDomainHeight - m.SliderZoneSpan));
			}

			clipArea->Panel().InvalidateLayout(false);
			clientOffsetArea->Panel().SetParentOffset({ 0, -parentOffset });
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

		EQueryInterfaceResult QueryInterface(IGRBase** ppOutputArg, cstr interfaceId) override
		{
			return Gui::QueryForParticularInterface<IGRWidgetViewport>(this, ppOutputArg, interfaceId);
		}

		int lastKnownDomainHeight = 0;

		void SetDomainHeight(int32 heightInPixels) override
		{
			if (lastKnownDomainHeight != heightInPixels)
			{
				lastKnownDomainHeight = heightInPixels;
				clientOffsetArea->Panel().Resize({ ClientArea().Panel().Span().x, max(1, heightInPixels) });
			}
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