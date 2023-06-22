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
	struct GRVerticalScroller : IGRWidgetVerticalScroller, IGRWidget
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

		void Layout(const GuiRect& panelDimensions) override
		{
			if (panel.EnumerateChildren(nullptr) != 0)
			{
				panel.Root().Custodian().RaiseError(EGRErrorCode::Generic, __FUNCTION__, "Vertical scrollbars should not have children");
			}

			int32 width = Width(panelDimensions);

			sliderZone.left = panelDimensions.left + 1;
			sliderZone.right = panelDimensions.right - 1;
			sliderZone.top = panelDimensions.top + 1;
			sliderZone.bottom = panelDimensions.bottom - 1;

			auto spec = events.OnCalculateSliderRect(Height(sliderZone), *this);
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

		void MovePage(int delta)
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

		int clickPosition = -1;
		int clickDeltaPosition = 0;

		void OnSliderSelected(GRCursorEvent& ce)
		{
			clickPosition = ce.position.y;
			panel.CaptureCursor();
		}

		EGREventRouting OnCursorClick(GRCursorEvent& ce) override
		{
			GuiRect sliderRect = ComputeSliderRect();

			if (ce.click.LeftButtonDown)
			{
				clickTarget = ClassifyTarget(ce.position);
				if (clickTarget == EClick::Slider)
				{
					OnSliderSelected(ce);
				}
			}
			else if (ce.click.LeftButtonUp)
			{
				clickPosition = -1;

				panel.Root().ReleaseCursor();

				if (clickDeltaPosition != 0)
				{
					sliderPosition = clamp(sliderPosition + clickDeltaPosition, 0, Height(sliderZone) - sliderHeight);
				}

				clickDeltaPosition = 0;

				if (clickTarget == ClassifyTarget(ce.position))
				{
					ActivateTarget(ce.position.y);
				}
				clickTarget = EClick::None;
			}
			else if (ce.click.MouseVWheel)
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

			return EGREventRouting::Terminate;
		}

		EGREventRouting OnCursorMove(GRCursorEvent& ce) override
		{
			if (clickPosition >= 0)
			{
				int32 oldPosition = clickDeltaPosition;
				clickDeltaPosition = ce.position.y - clickPosition;
				if (clickDeltaPosition != 0)
				{
					if (oldPosition != clickDeltaPosition)
					{
						events.OnScrollerNewPositionCalculated(clickDeltaPosition, *this);
					}
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
			bool isLit = IsPointInRect(g.CursorHoverPoint(), rect) || clickPosition >= 0;

			GRRenderState rs(clickPosition >= 0, IsPointInRect(g.CursorHoverPoint(), rect), false);

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
			bool isLit = IsPointInRect(g.CursorHoverPoint(), rect) || clickPosition >= 0;

			GRRenderState rs(clickPosition >= 0, IsPointInRect(g.CursorHoverPoint(), rect), false);

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

			int32 dy = clamp(sliderPosition + clickDeltaPosition, 0, Height(sliderZone) - sliderHeight);
			int32 y = sliderZone.top + 1 + dy;
			return { sliderZone.left + 1, y, sliderZone.right - 1, y + sliderHeight };
		}

		void Render(IGRRenderContext& g) override
		{
			auto rect = panel.AbsRect();

			GRRenderState rs(false, g.IsHovered(panel), false);

			RGBAb backColour = panel.GetColour(EGRSchemeColourSurface::SCROLLER_BAR_BACKGROUND, rs);
			g.DrawRect(rect, backColour);

			RGBAb edge1Colour = panel.GetColour(EGRSchemeColourSurface::SCROLLER_BAR_TOP_LEFT, rs);
			RGBAb edge2Colour = panel.GetColour(EGRSchemeColourSurface::SCROLLER_BAR_BOTTOM_RIGHT, rs);
			g.DrawRectEdge(rect, edge1Colour, edge2Colour);

			if (rs.value.intValue != 0)
			{
				g.DrawRectEdge(sliderZone, edge1Colour, edge2Colour);
			}

			RenderScrollerSlider(g, ComputeSliderRect());
		}

		EGREventRouting OnChildEvent(GRWidgetEvent& widgetEvent, IGRWidget& sourceWidget) override
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
		}

		EGREventRouting OnKeyEvent(GRKeyEvent& keyEvent) override
		{
			switch (keyEvent.osKeyEvent.VKey)
			{
			case IO::VKCode_PGUP:
				if (keyEvent.osKeyEvent.IsUp()) events.OnScrollPages(-1, *this);
				break;
			case IO::VKCode_PGDOWN:
				if (keyEvent.osKeyEvent.IsUp()) events.OnScrollPages(1, *this);
				break;
			case IO::VKCode_UP:
				if (keyEvent.osKeyEvent.IsUp()) events.OnScrollLines(-1, *this);
				break;
			case IO::VKCode_DOWN:
				if (keyEvent.osKeyEvent.IsUp()) events.OnScrollLines(1, *this);
				break;
			case IO::VKCode_HOME:
				if (keyEvent.osKeyEvent.IsUp()) events.OnScrollPages(-100, *this);
				break;
			case IO::VKCode_END:
				if (keyEvent.osKeyEvent.IsUp()) events.OnScrollPages(100, *this);
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
			m.PixelPosition = sliderPosition;
			m.SliderZoneSpan = Height(sliderZone);
			m.PixelRange = clamp(m.SliderZoneSpan - sliderHeight, 0, (int32) MAX_SCROLL_INT);
			return m;
		}

		void SetSliderPosition(int position) override
		{
			if (position < 0 || position > MAX_SCROLL_INT)
			{
				panel.Root().Custodian().RaiseError(EGRErrorCode::InvalidArg, __FUNCTION__, "Position was out of bounds");
				return;
			}
			this->sliderPosition = position;
		}

		EGRQueryInterfaceResult QueryInterface(IGRBase** ppOutputArg, cstr interfaceId) override
		{
			return Gui::QueryForParticularInterface<IGRWidgetVerticalScroller>(this, ppOutputArg, interfaceId);
		}

		IGRWidget& Widget() override
		{
			return *this;
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
		auto* scrollerClass = static_cast<ANON::GRVerticalScroller*>(scroller);
		return *scroller;
	}
}