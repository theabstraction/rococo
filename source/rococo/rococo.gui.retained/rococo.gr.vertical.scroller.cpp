#include <rococo.gui.retained.ex.h>
#include <rococo.maths.i32.h>
#include <rococo.maths.h>
#include <rococo.ui.h>
#include <rococo.vkeys.h>

namespace Rococo::Windows
{
	// Translates wheelDelta changes from the mouse wheel into MoveLine deltas, and emits a boolean true, if the deltas should be interpreted as page deltas
	ROCOCO_API int32 WheelDeltaToScrollLines(int32 wheelDelta, bool& scrollByPage);
}

using namespace Rococo;
using namespace Rococo::Gui;

namespace ANON
{
	struct GRVerticalScroller : IGRWidgetVerticalScroller, IGRWidgetSupervisor, IGRWidgetLayout
	{
		IGRPanel& panel;
		IGRScrollerEvents& events;

		GuiRect sliderZone{ 0,0,0,0 };

		int32 sliderPosition = 0; // Number of pixels below the top of the slider bar that the slider is set at.
		int32 sliderHeight = 10; // Number of pixels from the top to the bottom of the slider button

		GRVerticalScroller(IGRPanel& owningPanel, IGRScrollerEvents& argEvents) : panel(owningPanel), events(argEvents)
		{
		}

		void Free() override
		{
			delete this;
		}

		void LayoutBeforeFit() override
		{

		}

		void LayoutBeforeExpand() override
		{


		}

		void LayoutAfterExpand() override
		{
			int vpadding = 1;
			auto spec = events.OnCalculateSliderRect(panel.Span().y - (2 * vpadding), *this);
			sliderHeight = spec.sliderSpanInPixels;
		}

		enum class EClick
		{
			None,
			Slider,
			Bar,
		} clickTarget = EClick::None;

		EClick ClassifyTarget(Vec2i pos)
		{
			GuiRect sliderRect = ComputeSliderRect();

			if (IsPointInRect(pos, sliderRect))
			{
				return EClick::Slider;
			}
			else if (IsPointInRect(pos, sliderZone))
			{
				return EClick::Bar;
			}
			else
			{
				return EClick::None;
			}
		}

		void MovePage(int delta) override
		{
			events.OnScrollPages(delta, *this);
		}

		void ActivateTarget(int y)
		{
			switch (clickTarget)
			{
			case EClick::Bar:
				{
					GuiRect sliderRect = ComputeSliderRect();
					if (y < sliderRect.top)
					{
						MovePage(-1);
					}
					else if (y > sliderRect.bottom)
					{
						MovePage(1);
					}
				}
				break;
			}
		}

		bool isDragging = false;
		int clickPosition = -1;
		int deltaClickPosition = 0;

		void BeginDrag(GRCursorEvent& ce)
		{
			isDragging = true;
			clickPosition = ce.position.y;
			panel.CaptureCursor();
		}

		int ComputeDraggedSliderPosition() const
		{
			return clamp(sliderPosition + deltaClickPosition, 0, Height(sliderZone) - sliderHeight);
		}

		EGREventRouting OnCursorClick(GRCursorEvent& ce) override
		{
			GuiRect sliderRect = ComputeSliderRect();

			if (ce.click.LeftButtonDown)
			{
				panel.Focus();
				clickTarget = ClassifyTarget(ce.position);
				if (clickTarget == EClick::Slider)
				{
					BeginDrag(ce);
				}
			}
			else if (ce.click.LeftButtonUp)
			{
				isDragging = false;

				panel.Root().ReleaseCursor();

				if (deltaClickPosition != 0)
				{
					sliderPosition = ComputeDraggedSliderPosition();
				}

				deltaClickPosition = 0;

				if (clickTarget == ClassifyTarget(ce.position))
				{
					ActivateTarget(ce.position.y);
				}
				clickTarget = EClick::None;

				GRWidgetEvent sliderReleased;
				sliderReleased.eventType = EGRWidgetEventType::SCROLLER_RELEASED;
				sliderReleased.isCppOnly = true;
				sliderReleased.iMetaData = sliderPosition;
				sliderReleased.sMetaData = nullptr;
				panel.NotifyAncestors(sliderReleased, *this);
			}
			else if (ce.click.MouseVWheel)
			{
				if (!ce.context.isShiftHeld)
				{
					bool scrollByPage = false;
					int32 delta = Rococo::Windows::WheelDeltaToScrollLines(ce.wheelDelta, OUT scrollByPage) * panel.Root().GR().Config().VerticalScrollerWheelScaling;
					if (scrollByPage)
					{
						events.OnScrollPages(delta, *this);
					}
					else
					{
						events.OnScrollLines(delta, *this);
					}
				}
				else
				{
					return EGREventRouting::NextHandler;
				}
			}

			return EGREventRouting::Terminate;
		}

		EGREventRouting OnCursorMove(GRCursorEvent& ce) override
		{
			if (isDragging)
			{
				int32 oldPosition = deltaClickPosition;
				deltaClickPosition = ce.position.y - clickPosition;
				
				if (oldPosition != deltaClickPosition)
				{
					events.OnScrollerNewPositionCalculated(ComputeDraggedSliderPosition(), *this);
				}
			}
			return EGREventRouting::Terminate;
		}

		IGRPanel& Panel() override
		{
			return panel;
		}

		void RenderScrollerButton(IGRRenderContext& g, const GuiRect& rect, bool isUp)
		{
			GRWidgetRenderState rs(g.IsHovered(panel), IsPointInRect(g.CursorHoverPoint(), rect), panel.HasFocus() && IsPointInRect(g.CursorHoverPoint(), rect));

			RGBAb backColour = panel.GetColour(EGRSchemeColourSurface::SCROLLER_BUTTON_BACKGROUND, rs);
			g.DrawRect(rect, backColour);

			GuiRect triangleRect = { rect.left + 2, rect.top + 2, rect.right - 2, rect.bottom - 2 };

			RGBAb triangleColour = panel.GetColour(EGRSchemeColourSurface::SCROLLER_TRIANGLE_NORMAL, rs);
			g.DrawDirectionArrow(triangleRect, triangleColour, isUp ? 0.0_degrees : 180.0_degrees);

			RGBAb edge1Colour = panel.GetColour(EGRSchemeColourSurface::SCROLLER_BUTTON_TOP_LEFT, rs);
			RGBAb edge2Colour = panel.GetColour(EGRSchemeColourSurface::SCROLLER_BUTTON_BOTTOM_RIGHT, rs);
			g.DrawRectEdge(rect, edge1Colour, edge2Colour);
		}

		void RenderScrollerSlider(IGRRenderContext& g, const GuiRect& rect)
		{
			GRWidgetRenderState rs(g.IsHovered(panel), IsPointInRect(g.CursorHoverPoint(), rect), panel.HasFocus());

			RGBAb backColour = panel.GetColour(EGRSchemeColourSurface::SCROLLER_SLIDER_BACKGROUND, rs);
			g.DrawRect(rect, backColour);

			RGBAb edge1Colour = panel.GetColour(EGRSchemeColourSurface::SCROLLER_SLIDER_TOP_LEFT, rs);
			RGBAb edge2Colour = panel.GetColour(EGRSchemeColourSurface::SCROLLER_SLIDER_BOTTOM_RIGHT, rs);
			g.DrawRectEdge(rect, edge1Colour, edge2Colour);
		}

		GuiRect ComputeSliderRect() const
		{
			if (sliderHeight <= 0 || sliderHeight > Height(sliderZone) - 3)
			{
				return { 0,0,0,0 };
			}

			int32 dy = clamp(sliderPosition + deltaClickPosition, 0, Height(sliderZone) - sliderHeight);
			int32 y = sliderZone.top + 1 + dy;
			return { sliderZone.left + 1, y, sliderZone.right - 1, y + sliderHeight };
		}

		void Render(IGRRenderContext& g) override
		{
			auto rect = panel.AbsRect();

			GRWidgetRenderState rs(false, g.IsHovered(panel), false);

			RGBAb backColour = panel.GetColour(EGRSchemeColourSurface::SCROLLER_BAR_BACKGROUND, rs);
			g.DrawRect(rect, backColour);

			RGBAb edge1Colour = panel.GetColour(EGRSchemeColourSurface::SCROLLER_BAR_TOP_LEFT, rs);
			RGBAb edge2Colour = panel.GetColour(EGRSchemeColourSurface::SCROLLER_BAR_BOTTOM_RIGHT, rs);
			g.DrawRectEdge(rect, edge1Colour, edge2Colour);

			sliderZone.left = rect.left + 1;
			sliderZone.right = rect.right - 1;
			sliderZone.top = rect.top + 1;
			sliderZone.bottom = rect.bottom - 1;

			if (rs.value.intValue != 0)
			{
				g.DrawRectEdge(sliderZone, edge1Colour, edge2Colour);
			}

			GuiRect renderedSliderRect = ComputeSliderRect();
			renderedSliderRect.bottom = min(rect.bottom - 2, renderedSliderRect.bottom);
			
			RenderScrollerSlider(g, renderedSliderRect);
		}

		EGREventRouting OnChildEvent(GRWidgetEvent&, IGRWidget&) override
		{
			return EGREventRouting::NextHandler;
		}

		void OnCursorEnter() override
		{
		}

		void OnCursorLeave() override
		{
			clickTarget = EClick::None;
			clickPosition = -1;
			isDragging = false;
		}

		EGREventRouting OnKeyEvent(GRKeyEvent& keyEvent) override
		{
			switch (keyEvent.osKeyEvent.VKey)
			{
			case IO::VirtualKeys::VKCode_PGUP:
				if (!keyEvent.osKeyEvent.IsUp()) events.OnScrollPages(-1, *this);
				break;
			case IO::VirtualKeys::VKCode_PGDOWN:
				if (!keyEvent.osKeyEvent.IsUp()) events.OnScrollPages(1, *this);
				break;
			case IO::VirtualKeys::VKCode_UP:
				if (!keyEvent.osKeyEvent.IsUp()) events.OnScrollLines(-1, *this);
				break;
			case IO::VirtualKeys::VKCode_DOWN:
				if (!keyEvent.osKeyEvent.IsUp()) events.OnScrollLines(1, *this);
				break;
			case IO::VirtualKeys::VKCode_HOME:
				if (!keyEvent.osKeyEvent.IsUp()) events.OnScrollPages(-100, *this);
				break;
			case IO::VirtualKeys::VKCode_END:
				if (!keyEvent.osKeyEvent.IsUp()) events.OnScrollPages(100, *this);
				break;
			default:
				return EGREventRouting::NextHandler;
			}
			return EGREventRouting::Terminate;
		}

		enum { MAX_SCROLL_INT = 1 << 30 };

		GRScrollerMetrics GetMetrics() const override
		{
			GRScrollerMetrics m;
			m.SliderTopPosition = sliderPosition;
			m.SliderZoneSpan = Height(sliderZone);
			m.PixelRange = clamp(m.SliderZoneSpan - sliderHeight, 0, (int32) MAX_SCROLL_INT);
			return m;
		}

		void SetSliderPosition(int position) override
		{
			if (position < 0)
			{
				auto m = GetMetrics();
				this->sliderPosition = m.PixelRange;
			}

			if (position > MAX_SCROLL_INT)
			{
				RaiseError(panel, EGRErrorCode::InvalidArg, __ROCOCO_FUNCTION__, "Position was out of bounds");
				return;
			}
			this->sliderPosition = position;
		}

		EGRQueryInterfaceResult QueryInterface(IGRBase** ppOutputArg, cstr interfaceId) override
		{
			auto result = Gui::QueryForParticularInterface<IGRWidgetLayout>(this, ppOutputArg, interfaceId);
			if (result == EGRQueryInterfaceResult::SUCCESS)
			{
				return result;
			}

			return Gui::QueryForParticularInterface<IGRWidgetVerticalScroller>(this, ppOutputArg, interfaceId);
		}

		IGRWidget& Widget() override
		{
			return *this;
		}

		cstr GetImplementationTypeName() const override
		{
			return "GRVerticalScroller";
		}
	};

	struct VerticalScrollerFactory : IGRWidgetFactory
	{
		IGRScrollerEvents& events;

		VerticalScrollerFactory(IGRScrollerEvents& argEvents): events(argEvents)
		{

		}

		IGRWidget& CreateWidget(IGRPanel& panel)
		{
			return *new GRVerticalScroller(panel, events);
		}
	};
}

namespace Rococo::Gui
{
	ROCOCO_GUI_RETAINED_API cstr IGRWidgetVerticalScroller::InterfaceId()
	{
		return "IGRWidgetVerticalScroller";
	}

	ROCOCO_GUI_RETAINED_API IGRWidgetVerticalScroller& CreateVerticalScroller(IGRWidget& parent, IGRScrollerEvents& events)
	{
		ANON::VerticalScrollerFactory factory(events);

		auto& gr = parent.Panel().Root().GR();
		auto* scroller = Cast<IGRWidgetVerticalScroller>(gr.AddWidget(parent.Panel(), factory));
		return *scroller;
	}
}