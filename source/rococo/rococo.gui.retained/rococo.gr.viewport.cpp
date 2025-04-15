#include <rococo.gui.retained.ex.h>
#include <rococo.maths.i32.h>
#include <stdio.h>

using namespace Rococo;
using namespace Rococo::Gui;

namespace ANON
{
	enum { scrollbarWidth = 16 };

	struct GRViewportWidget : IGRWidgetViewport, IGRWidgetSupervisor, IGRScrollerEvents, IGRFocusNotifier, IGRWidgetLayout
	{
		IGRPanel& panel;
		IGRWidgetDivision* clipArea = nullptr; // Represents the rectangle to the left of the scroller
		IGRWidgetDivision* clientOffsetArea = nullptr; // Represents the scrolled data inside the clipArea
		IGRWidgetVerticalScrollerWithButtons* vscroller = nullptr;
		int clientOffsetAreaParentOffset = 0;

		GRViewportWidget(IGRPanel& owningPanel) : panel(owningPanel)
		{
			owningPanel.SetMinimalSpan({ 10, 10 });
			owningPanel.SetLayoutDirection(ELayoutDirection::LeftToRight);
		}

		void PostConstruct()
		{
			clipArea = &CreateDivision(*this);
			clipArea->Panel().SetExpandToParentVertically();
			clipArea->Panel().SetExpandToParentHorizontally();
			clipArea->Panel().SetLayoutDirection(ELayoutDirection::None);
			clientOffsetArea = &CreateDivision(clipArea->Widget());
			clientOffsetArea->Panel().SetExpandToParentVertically();
			clientOffsetArea->Panel().SetExpandToParentHorizontally();
			clientOffsetArea->Panel().SetClippingPanel(&clipArea->Panel());
			vscroller = &CreateVerticalScrollerWithButtons(*this, *this);
			vscroller->Widget().Panel().SetExpandToParentVertically();
			vscroller->Widget().Panel().SetConstantWidth(scrollbarWidth);
		}

		void Free() override
		{
			delete this;
		}

		void LayoutBeforeFit() override
		{
			SetDomainHeight(nextDomain);
		}

		void LayoutBeforeExpand() override
		{

		}

		void LayoutAfterExpand() override
		{
			auto span = panel.Span();

			int trueScrollBarWidth = scrollbarWidth;

			if (nextDomain <= panel.Span().y)
			{
				vscroller->Panel().SetCollapsed(true);
				trueScrollBarWidth = 0;
			}
			else
			{
				vscroller->Panel().SetCollapsed(false);
			}

			Vec2i clipSpan{ span.x - trueScrollBarWidth, span.y };
			auto& panel = clipArea->Panel();
			panel.SetConstantWidth(clipSpan.x);
			panel.SetConstantHeight(clipSpan.y);
			panel.SetParentOffset({ 0,0 });
			
			Vec2i clientOffsetSpan{ clipSpan.x, max(lastKnownDomainHeight, clipSpan.y) };

			auto& coaPanel = clientOffsetArea->Panel();
			coaPanel.SetConstantWidth(clientOffsetSpan.x);
			coaPanel.SetConstantHeight(clientOffsetSpan.y);
			coaPanel.SetParentOffset({ 0, -clientOffsetAreaParentOffset });
			
			auto& vswp = vscroller->Widget().Panel();
			vswp.SetConstantWidth(trueScrollBarWidth);
			vswp.SetConstantHeight(span.y);
			vswp.SetParentOffset({ span.x - trueScrollBarWidth, 1 });
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
			if (m.PixelRange == 0)
			{
				OnScrollerNewPositionCalculated(0, scroller);
				scroller.SetSliderPosition(0);
			}
			else if (m.PixelRange > 0 && lastKnownDomainHeight > m.SliderZoneSpan)
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

				int32 deltaPixels = delta * clipArea->Panel().Span().y;

				int newPosition = (int)(pageDeltaScale * deltaPixels / scale);

				OnScrollerNewPositionCalculated(newPosition, scroller);

				scroller.SetSliderPosition(clamp(newPosition + m.PixelPosition, 0, m.PixelRange));
			}
		}

		void OnScrollerNewPositionCalculated(int32 newPosition, IGRWidgetScroller& scroller) override
		{
			GRScrollerMetrics m = scroller.GetMetrics();
			if (m.PixelRange == 0)
			{
				clientOffsetAreaParentOffset = 0;
			}
			else if (m.PixelRange > 0 && lastKnownDomainHeight > m.SliderZoneSpan)
			{
				double cursor = clamp((double)(m.PixelPosition + newPosition) / (double)m.PixelRange, 0.0, 1.0);
				clientOffsetAreaParentOffset = (int)(cursor * (lastKnownDomainHeight - panel.Span().y));
			}
		}

		EGREventRouting OnCursorClick(GRCursorEvent& ce) override
		{
			if (ce.wheelDelta != 0)
			{
				auto& vscrollerSuper = static_cast<IGRWidgetSupervisor&>(vscroller->Scroller().Widget());
				vscrollerSuper.OnCursorClick(ce);
				return EGREventRouting::Terminate;
			}

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
			auto clipRect = clipArea->Panel().AbsRect();

			int32 dOffset = 0;
			if (rect.bottom < clipArea->Panel().AbsRect().top)
			{
				dOffset = rect.bottom - clipArea->Panel().AbsRect().top;
			}
			else if (rect.top > clipArea->Panel().AbsRect().bottom)
			{
				if (Height(rect) < Height(clipRect))
				{
					dOffset = clipArea->Panel().AbsRect().top - rect.top;
				}
				else
				{
					dOffset = clipArea->Panel().AbsRect().bottom - rect.top - 30;
				}
			}
			else
			{
				return;
			}

			clientOffsetAreaParentOffset = clientOffsetArea->Panel().ParentOffset().y + dOffset;

			GRScrollerMetrics m = vscroller->Scroller().GetMetrics();
			if (m.PixelRange > 0 && lastKnownDomainHeight > m.SliderZoneSpan)
			{
				double sliderPixelOffset = -(double) (m.PixelRange * clientOffsetAreaParentOffset) / (double) (lastKnownDomainHeight - m.SliderZoneSpan);
				vscroller->Scroller().SetSliderPosition(clamp((int32)sliderPixelOffset, 0, m.PixelRange));
			}
		}

		void OnDeepChildFocusSet(int64 panelId) override
		{
			auto* w = panel.Root().GR().FindWidget(panelId);
			if (!w)
			{
				return;
			}

			if (IsCandidateDescendantOfParent(clientOffsetArea->Panel(), w->Panel()))
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

		void Render(IGRRenderContext&) override
		{
		}

		int nextDomain = 0;

		EGREventRouting OnChildEvent(GRWidgetEvent& ev, IGRWidget&) override
		{
			switch (ev.eventType)
			{
			case EGRWidgetEventType::UPDATED_CLIENTAREA_HEIGHT:
				nextDomain = (int) ev.iMetaData;
				return EGREventRouting::Terminate;
			}
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

			auto result = Gui::QueryForParticularInterface<IGRWidgetLayout>(this, ppOutputArg, interfaceId);
			if (result == EGRQueryInterfaceResult::SUCCESS)
			{
				return result;
			}

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
				nextDomain = heightInPixels;
				lastKnownDomainHeight = heightInPixels;
				clientOffsetArea->Panel().SetConstantHeight(max(1, heightInPixels));
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