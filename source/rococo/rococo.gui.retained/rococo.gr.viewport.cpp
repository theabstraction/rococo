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
			owningPanel.SetLayoutDirection(ELayoutDirection::None);
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
			if (syncDomainToChildren)
			{
				auto& clientPanel = ClientArea().Panel();

				nextDomain = clientPanel.Padding().top;

				int nChildren = clientPanel.EnumerateChildren(nullptr);

				for (int i = 0; i < nChildren; i++)
				{
					auto* child = clientPanel.GetChild(i);
					nextDomain += child->Span().y;
				}

				if (nChildren > 1)
				{
					nextDomain += (nChildren - 1) * clientPanel.ChildPadding();
				}

				nextDomain += clientPanel.Padding().bottom;
			}

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
				ClientArea().Panel().SetRectStyle(notScrollableRectStyle);
				panel.SetRectStyle(notScrollableRectStyle);
			}
			else
			{
				vscroller->Panel().SetCollapsed(false);
				ClientArea().Panel().SetRectStyle(scrollableRectStyle);
				panel.SetRectStyle(scrollableRectStyle);
			}

			Vec2i roundedOffset = { 0,0 };

			Vec2i clipSpan{ span.x - trueScrollBarWidth, span.y };

			if (panel.RectStyle() != EGRRectStyle::SHARP)
			{
				roundedOffset = { panel.CornerRadius(), 0 };
				trueScrollBarWidth = 2 * roundedOffset.x;

				clipSpan.x -= trueScrollBarWidth;
			}

			auto& clipPanel = clipArea->Panel();
			clipPanel.SetConstantWidth(clipSpan.x);
			clipPanel.SetConstantHeight(clipSpan.y);
			clipPanel.SetParentOffset({roundedOffset.x,0});
			
			Vec2i clientOffsetSpan{ clipSpan.x, max(lastKnownDomainHeight, clipSpan.y) };

			auto& coaPanel = clientOffsetArea->Panel();
			coaPanel.SetConstantWidth(clientOffsetSpan.x);
			coaPanel.SetConstantHeight(clientOffsetSpan.y);
			coaPanel.SetParentOffset({0, -clientOffsetAreaParentOffset });
			
			auto& vswp = vscroller->Widget().Panel();
			vswp.SetConstantWidth(trueScrollBarWidth);
			vswp.SetConstantHeight(clipSpan.y);

			vswp.SetCornerRadius(panel.CornerRadius());
			vswp.SetRectStyle(panel.RectStyle());

			int scrollerX = panel.Span().x - trueScrollBarWidth;
			vswp.SetParentOffset({ scrollerX, 1});
		}

		GRSliderSpec OnCalculateSliderRect(int32 scrollerSpan, IGRWidgetScroller&) override
		{
			double sliderSpanToScrollerSpan = scrollerSpan / (double)lastKnownDomainHeight;

			GRSliderSpec spec;
			spec.sliderSpanInPixels = (int) (clipArea->Panel().Span().y * sliderSpanToScrollerSpan);
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

		int GetOffset() const override
		{
			return clientOffsetAreaParentOffset;
		}

		void SetOffset(int offset, bool fromStart) override
		{
			int span = lastKnownDomainHeight - Height(clipArea->Panel().AbsRect());

			if (span < 0)
			{
				clientOffsetAreaParentOffset = 0;
				return;
			}

			if (offset < 0)
			{
				clientOffsetAreaParentOffset = max(0, span);
				return;
			}

			if (fromStart)
			{
				clientOffsetAreaParentOffset = offset;
			}
			else
			{
				clientOffsetAreaParentOffset = span - offset;
			}

			clientOffsetAreaParentOffset = clamp(clientOffsetAreaParentOffset, 0, span);
		}

		EGRRectStyle notScrollableRectStyle = EGRRectStyle::SHARP;

		void SetClientAreaRectStyleWhenNotScrollable(EGRRectStyle style) override
		{
			notScrollableRectStyle = style;
		}

		EGRRectStyle scrollableRectStyle = EGRRectStyle::SHARP;

		void SetClientAreaRectStyleWhenScrollable(EGRRectStyle style) override
		{
			scrollableRectStyle = style;
		}

		bool syncDomainToChildren = false;

		void SyncDomainToChildren()
		{
			syncDomainToChildren = true;
		}

		void SetMovePageScale(double scaleFactor) override
		{
			pageDeltaScale = clamp(scaleFactor, 0.0, 2.0);
		}

		void ScrollDeltaPixels(int deltaPixels, IGRWidgetScroller& scroller)
		{
			// panel.Root().GR().SetFocus(-1);

			int clipAreaHeight = clipArea->Panel().Span().y;

			GRScrollerMetrics m = scroller.GetMetrics();
			if (m.PixelRange > 0 && lastKnownDomainHeight > clipAreaHeight)
			{
				double scale = m.PixelRange / (double)(lastKnownDomainHeight - clipAreaHeight);
				int32 newOffset = deltaPixels + clientOffsetAreaParentOffset;

				if (lineDeltaPixels > 0 && lineDeltaPixels < deltaPixels)
				{
					newOffset = (newOffset / lineDeltaPixels) * lineDeltaPixels;
				}

				clientOffsetAreaParentOffset = clamp(0, newOffset, lastKnownDomainHeight - clipAreaHeight);

				int newPosition = (int)(newOffset * scale);
				newPosition = clamp(newPosition, 0, m.PixelRange);

				scroller.SetSliderPosition(newPosition);
			}
		}

		void OnScrollLines(int delta, IGRWidgetScroller& scroller)
		{
			if (lineDeltaPixels <= 0) return;
			int32 deltaPixels = delta * lineDeltaPixels;
			ScrollDeltaPixels(deltaPixels, scroller);
		}

		void OnScrollPages(int delta, IGRWidgetScroller& scroller) override
		{
			int clipAreaHeight = clipArea->Panel().Span().y;
			int deltaPixels = (int)(pageDeltaScale * delta * clipAreaHeight);
			ScrollDeltaPixels(deltaPixels, scroller);
		}

		void AdjustClientOffsetAreaAccordingToNewPosition(int newPosition, const GRScrollerMetrics m)
		{
			int clipAreaHeight = clipArea->Panel().Span().y;

			if (m.PixelRange == 0)
			{
				clientOffsetAreaParentOffset = 0;
			}
			else if (m.PixelRange > 0 && lastKnownDomainHeight > clipAreaHeight)
			{
				double cursor = clamp(newPosition / (double)(m.PixelRange), 0.0, 1.0);
				clientOffsetAreaParentOffset = (int)(cursor * (lastKnownDomainHeight - clipAreaHeight));
			}
		}

		void OnScrollerNewPositionCalculated(int32 newPosition, IGRWidgetScroller& scroller) override
		{
			GRScrollerMetrics m = scroller.GetMetrics();
			AdjustClientOffsetAreaAccordingToNewPosition(newPosition, m);
		}

		EGREventRouting OnCursorClick(GRCursorEvent& ce) override
		{
			if (ce.wheelDelta != 0)
			{
				vscroller->Scroller().Widget().Supervisor().OnCursorClick(ce);
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
			if (rect.top < clipArea->Panel().AbsRect().top)
			{
				dOffset = rect.top - clipArea->Panel().AbsRect().top;
			}
			else if (rect.top >= clipArea->Panel().AbsRect().bottom)
			{
				if (Height(rect) < Height(clipRect))
				{
					dOffset = clipArea->Panel().AbsRect().bottom - rect.bottom;
				}
				else
				{
					dOffset = clipArea->Panel().AbsRect().bottom - rect.top;
				}
			}
			else
			{
				return;
			}

			clientOffsetAreaParentOffset = - clientOffsetArea->Panel().ParentOffset().y + dOffset;

			GRScrollerMetrics m = vscroller->Scroller().GetMetrics();
			if (m.PixelRange > 0 && lastKnownDomainHeight > m.SliderZoneSpan)
			{
				double sliderPixelOffset = (double) (m.PixelRange * clientOffsetAreaParentOffset) / (double) (lastKnownDomainHeight - Span(clipArea->Panel().AbsRect()).y);
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

		void Render(IGRRenderContext& g) override
		{
			DrawPanelBackgroundEx(panel, g, EGRSchemeColourSurface::BACKGROUND, EGRSchemeColourSurface::CONTAINER_TOP_LEFT, EGRSchemeColourSurface::CONTAINER_BOTTOM_RIGHT);
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