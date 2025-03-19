#include <rococo.gui.retained.ex.h>
#include <rococo.maths.i32.h>

using namespace Rococo;
using namespace Rococo::Gui;

namespace ANON
{
	struct GRViewportWidget : IGRWidgetViewport, IGRWidgetSupervisor, IGRScrollerEvents, IGRFocusNotifier
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
			clientOffsetArea = &CreateDivision(clipArea->Widget());
			clientOffsetArea->Widget().Panel().PreventInvalidationFromChildren();
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
			auto& panel = clipArea->Widget().Panel();
			panel.Resize(clipSpan);
			panel.SetParentOffset({ 0,0 });
			panel.InvalidateLayout(false);

			Vec2i clientOffsetSpan{ clipSpan.x, max(lastKnownDomainHeight, clipSpan.y) };

			int parentOffset = 0;

			GRScrollerMetrics m = vscroller->Scroller().GetMetrics();
			if (m.PixelRange > 0)
			{
				double cursor = clamp((double)m.PixelPosition / (double)m.PixelRange, 0.0, 1.0);
				parentOffset = (int)(cursor * (lastKnownDomainHeight - m.SliderZoneSpan));
			}

			auto& coaPanel = clientOffsetArea->Widget().Panel();
			coaPanel.Resize(clientOffsetSpan);
			coaPanel.SetParentOffset({ 0, -parentOffset });
			coaPanel.InvalidateLayout(false);
			InvalidateLayoutForAllDescendants(coaPanel);

			vscroller->Widget().Panel().Resize({ scrollbarWidth, Height(panelDimensions) - 2 });
			vscroller->Widget().Panel().SetParentOffset({ Width(panelDimensions) - scrollbarWidth, 1 });
			vscroller->Widget().Panel().InvalidateLayout(false);
		}

		GRSliderSpec OnCalculateSliderRect(int32 scrollerSpan, IGRWidgetScroller&) override
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

		void OnScrollLines(int delta, IGRWidgetScroller& scroller)
		{
			if (lineDeltaPixels == 0) return;

			GRScrollerMetrics m = scroller.GetMetrics();
			if (m.PixelRange > 0 && lastKnownDomainHeight > m.SliderZoneSpan)
			{
				double scale = (lastKnownDomainHeight - m.SliderZoneSpan) / (double)m.PixelRange;

				int32 deltaPixels = delta * lineDeltaPixels;

				int newPosition = (int)(deltaPixels / scale);

				if (newPosition == 0)
				{
					// Don't let the scaling completely eliminate the change, we must always have some scrolling
					newPosition = (delta > 0) ? 1 : -1;
				}

				OnScrollerNewPositionCalculated(newPosition, scroller);

				scroller.SetSliderPosition(clamp(newPosition + m.PixelPosition, 0, m.PixelRange));
			}
		}

		void OnScrollPages(int delta, IGRWidgetScroller& scroller) override
		{
			GRScrollerMetrics m = scroller.GetMetrics();
			if (m.PixelRange > 0 && lastKnownDomainHeight > m.SliderZoneSpan)
			{
				double scale = (lastKnownDomainHeight - m.SliderZoneSpan) / (double)m.PixelRange;

				int32 deltaPixels = delta * clipArea->Widget().Panel().Span().y;

				int newPosition = (int)(pageDeltaScale * deltaPixels / scale);

				OnScrollerNewPositionCalculated(newPosition, scroller);

				scroller.SetSliderPosition(clamp(newPosition + m.PixelPosition, 0, m.PixelRange));
			}
		}

		void OnScrollerNewPositionCalculated(int32 newPosition, IGRWidgetScroller& scroller) override
		{
			int parentOffset = 0;

			GRScrollerMetrics m = scroller.GetMetrics();
			if (m.PixelRange > 0 && lastKnownDomainHeight > m.SliderZoneSpan)
			{
				double cursor = clamp((double)(m.PixelPosition + newPosition) / (double)m.PixelRange, 0.0, 1.0);
				parentOffset = (int)(cursor * (lastKnownDomainHeight - m.SliderZoneSpan));
			}

			clipArea->Widget().Panel().InvalidateLayout(false);
			clientOffsetArea->Widget().Panel().SetParentOffset({ 0, -parentOffset });
			clientOffsetArea->Widget().Panel().InvalidateLayout(false);
			InvalidateLayoutForAllDescendants(clientOffsetArea->Widget().Panel());
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

		void ScrollIntoView(const GuiRect& rect)
		{
			auto clipRect = clipArea->Widget().Panel().AbsRect();

			int32 dOffset = 0;
			if (rect.bottom < clipArea->Widget().Panel().AbsRect().top)
			{
				dOffset = rect.bottom - clipArea->Widget().Panel().AbsRect().top;
			}
			else if (rect.top > clipArea->Widget().Panel().AbsRect().bottom)
			{
				if (Height(rect) < Height(clipRect))
				{
					dOffset = clipArea->Widget().Panel().AbsRect().top - rect.top;
				}
				else
				{
					dOffset = clipArea->Widget().Panel().AbsRect().bottom - rect.top - 30;
				}
			}
			else
			{
				return;
			}

			int32 parentOffset = clientOffsetArea->Widget().Panel().ParentOffset().y + dOffset;

			GRScrollerMetrics m = vscroller->Scroller().GetMetrics();
			if (m.PixelRange > 0 && lastKnownDomainHeight > m.SliderZoneSpan)
			{
				double sliderPixelOffset = -(double) (m.PixelRange * parentOffset) / (double) (lastKnownDomainHeight - m.SliderZoneSpan);
				vscroller->Scroller().SetSliderPosition(clamp((int32)sliderPixelOffset, 0, m.PixelRange));
			}

			clipArea->Widget().Panel().InvalidateLayout(false);
			clientOffsetArea->Widget().Panel().SetParentOffset({ 0, parentOffset });
			clientOffsetArea->Widget().Panel().InvalidateLayout(false);
			InvalidateLayoutForAllDescendants(clientOffsetArea->Widget().Panel());
		}

		void OnDeepChildFocusSet(int64 panelId) override
		{
			auto* w = panel.Root().GR().FindWidget(panelId);
			if (!w)
			{
				return;
			}

			if (IsCandidateDescendantOfParent(clientOffsetArea->Widget().Panel(), w->Panel()))
			{
				auto rect = w->Panel().AbsRect();
				ScrollIntoView(rect);
			}
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
			if (ppOutputArg) *ppOutputArg = nullptr;
			if (!interfaceId || *interfaceId == 0) return EGRQueryInterfaceResult::INVALID_ID;

			if (DoInterfaceNamesMatch(interfaceId, IGRWidgetViewport::InterfaceId()))
			{
				if (ppOutputArg)
				{
					*ppOutputArg = static_cast<IGRWidgetViewport*>(this);
				}

				return EGRQueryInterfaceResult::SUCCESS;
			}
			else if (DoInterfaceNamesMatch(interfaceId, IGRFocusNotifier::InterfaceId()))
			{
				if (ppOutputArg)
				{
					*ppOutputArg = static_cast<IGRFocusNotifier*>(this);
				}

				return EGRQueryInterfaceResult::SUCCESS;
			}

			return EGRQueryInterfaceResult::NOT_IMPLEMENTED;
		}

		int lastKnownDomainHeight = 0;

		void SetDomainHeight(int32 heightInPixels) override
		{
			if (lastKnownDomainHeight != heightInPixels)
			{
				lastKnownDomainHeight = heightInPixels;
				clientOffsetArea->Widget().Panel().Resize({ ClientArea().Widget().Panel().Span().x, max(1, heightInPixels) });
			}
		}

		IGRWidget& Widget() override
		{
			return *this;
		}

		cstr GetImplementationTypeName() const override
		{
			return "GRViewportWidget";
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