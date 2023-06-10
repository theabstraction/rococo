#include <rococo.gui.retained.ex.h>
#include <rococo.maths.i32.h>
#include <rococo.maths.h>

using namespace Rococo;
using namespace Rococo::Gui;

namespace ANON
{
	struct GRVerticalScroller : IGRWidgetVerticalScroller, IGRWidget
	{
		IGRPanel& panel;
		IScrollerEvents* events = nullptr; // this is always set to non-null in the factory function at the bottom of the file

		GuiRect sliderZone{ 0,0,0,0 };

		int32 sliderPosition = 0; // Number of pixels below the top of the slider bar that the slider is set at.
		int32 sliderHeight = 10; // Number of pixels from the top to the bottom of the slider button

		GRVerticalScroller(IGRPanel& owningPanel) : panel(owningPanel)
		{
		}

		void Free() override
		{
			delete this;
		}

		void SetEventHandler(IScrollerEvents& events)
		{
			this->events = &events;
		}

		void Layout(const GuiRect& panelDimensions) override
		{
			if (panel.EnumerateChildren(nullptr) != 0)
			{
				panel.Root().Custodian().RaiseError(GRErrorCode::Generic, __FUNCTION__, "Vertical scrollbars should not have children");
			}

			int32 width = Width(panelDimensions);

			sliderZone.left = panelDimensions.left + 1;
			sliderZone.right = panelDimensions.right - 1;
			sliderZone.top = panelDimensions.top + 1;
			sliderZone.bottom = panelDimensions.bottom - 1;
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

		void OnSliderSelected(CursorEvent& ce)
		{
			clickPosition = ce.position.y;
			panel.CaptureCursor();
		}

		EventRouting OnCursorClick(CursorEvent& ce) override
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

			return EventRouting::Terminate;
		}

		EventRouting OnCursorMove(CursorEvent& ce) override
		{
			if (clickPosition >= 0)
			{
				int32 oldPosition = clickDeltaPosition;
				clickDeltaPosition = ce.position.y - clickPosition;
				if (clickDeltaPosition > 0 && oldPosition != clickDeltaPosition)
				{
					events->OnScrollerNewPositionCalculated(clickDeltaPosition, *this);
				}
			}
			return EventRouting::Terminate;
		}

		IGRPanel& Panel() override
		{
			return panel;
		}

		void RenderScrollerButton(IGRRenderContext& g, const GuiRect& rect, bool isUp)
		{
			bool isHovered = IsPointInRect(g.CursorHoverPoint(), rect);

			RGBAb backColour = panel.GetColour(isHovered ? ESchemeColourSurface::SCROLLER_BUTTON_BACKGROUND_HOVERED : ESchemeColourSurface::SCROLLER_BUTTON_BACKGROUND);
			g.DrawRect(rect, backColour);

			GuiRect triangleRect = { rect.left + 2, rect.top + 2, rect.right - 2, rect.bottom - 2 };

			RGBAb triangleColour = panel.GetColour(isHovered ? ESchemeColourSurface::SCROLLER_TRIANGLE_HOVERED : ESchemeColourSurface::SCROLLER_TRIANGLE_NORMAL);
			g.DrawDirectionArrow(triangleRect, triangleColour, isUp ? 0.0_degrees : 180.0_degrees);

			RGBAb edge1Colour = panel.GetColour(isHovered ? ESchemeColourSurface::SCROLLER_BUTTON_TOP_LEFT_HOVERED : ESchemeColourSurface::SCROLLER_BUTTON_TOP_LEFT);
			RGBAb edge2Colour = panel.GetColour(isHovered ? ESchemeColourSurface::SCROLLER_BUTTON_BOTTOM_RIGHT_HOVERED : ESchemeColourSurface::SCROLLER_BUTTON_BOTTOM_RIGHT);
			g.DrawRectEdge(rect, edge1Colour, edge2Colour);
		}

		void RenderScrollerSlider(IGRRenderContext& g, const GuiRect& rect)
		{
			bool isHovered = IsPointInRect(g.CursorHoverPoint(), rect);

			RGBAb backColour = panel.GetColour(isHovered ? ESchemeColourSurface::SCROLLER_SLIDER_BACKGROUND_HOVERED : ESchemeColourSurface::SCROLLER_SLIDER_BACKGROUND);
			g.DrawRect(rect, backColour);

			RGBAb edge1Colour = panel.GetColour(isHovered ? ESchemeColourSurface::SCROLLER_SLIDER_TOP_LEFT_HOVERED : ESchemeColourSurface::SCROLLER_SLIDER_TOP_LEFT);
			RGBAb edge2Colour = panel.GetColour(isHovered ? ESchemeColourSurface::SCROLLER_SLIDER_BOTTOM_RIGHT_HOVERED : ESchemeColourSurface::SCROLLER_SLIDER_BOTTOM_RIGHT);
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
			bool isHovered = g.IsHovered(panel);	
			RGBAb edge1Colour = panel.GetColour(isHovered ? ESchemeColourSurface::SCROLLER_BAR_TOP_LEFT_HOVERED : ESchemeColourSurface::SCROLLER_BAR_TOP_LEFT);
			RGBAb edge2Colour = panel.GetColour(isHovered ? ESchemeColourSurface::SCROLLER_BAR_BOTTOM_RIGHT_HOVERED : ESchemeColourSurface::SCROLLER_BAR_BOTTOM_RIGHT);
			g.DrawRectEdge(rect, edge1Colour, edge2Colour);

			if (isHovered)
			{
				g.DrawRectEdge(sliderZone, edge1Colour, edge2Colour);
			}

			RenderScrollerSlider(g, ComputeSliderRect());
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
			clickTarget = EClick::None;
			clickPosition = -1;
		}

		EventRouting OnKeyEvent(KeyEvent& keyEvent) override
		{
			return EventRouting::NextHandler;
		}

		Vec2i EvaluateMinimalSpan() const override
		{
			return { 0,0 };
		}

		enum { MAX_SCROLL_INT = 1 << 30 };

		void GetMetrics(ScrollerMetrics& m) const override
		{
			m.PixelPosition = sliderPosition;
			m.PixelRange = clamp(sliderHeight - Height(sliderZone), 0, (int32) MAX_SCROLL_INT);
		}

		void SetSliderPosition(int position) override
		{
			if (position < 0 || position > MAX_SCROLL_INT)
			{
				panel.Root().Custodian().RaiseError(GRErrorCode::InvalidArg, __FUNCTION__, "Position was out of bounds");
				return;
			}
			this->sliderPosition = position;
		}

		void SetSliderHeight(int sliderHeight) override
		{
			if (sliderHeight < 0 || sliderHeight > MAX_SCROLL_INT)
			{
				panel.Root().Custodian().RaiseError(GRErrorCode::InvalidArg, __FUNCTION__, "Range was out of bounds");
				return;
			}
			this->sliderHeight = sliderHeight;
		}

		EQueryInterfaceResult QueryInterface(IGRBase** ppOutputArg, cstr interfaceId) override
		{
			return Gui::QueryForParticularInterface<IGRWidgetVerticalScroller>(this, ppOutputArg, interfaceId);
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
			return *new GRVerticalScroller(panel);
		}
	} s_VerticalScrollerFactory;
}

namespace Rococo::Gui
{
	ROCOCO_GUI_RETAINED_API cstr IGRWidgetVerticalScroller::InterfaceId()
	{
		return "IGRWidgetVerticalScroller";
	}

	ROCOCO_GUI_RETAINED_API IGRWidgetVerticalScroller& CreateVerticalScroller(IGRWidget& parent, IScrollerEvents& events)
	{
		auto& gr = parent.Panel().Root().GR();
		auto* scroller = Cast<IGRWidgetVerticalScroller>(gr.AddWidget(parent.Panel(), ANON::s_VerticalScrollerFactory));
		auto* scrollerClass = static_cast<ANON::GRVerticalScroller*>(scroller);
		scrollerClass->SetEventHandler(events);
		return *scroller;
	}
}