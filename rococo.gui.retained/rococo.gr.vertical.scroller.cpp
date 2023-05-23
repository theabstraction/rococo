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

		GuiRect topButton {0,0,0,0};
		GuiRect bottomButton {0,0,0,0};
		GuiRect sliderZone{ 0,0,0,0 };

		GRVerticalScroller(IGRPanel& owningPanel) : panel(owningPanel)
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
				panel.Root().Custodian().RaiseError(GRErrorCode::Generic, __FUNCTION__, "Vertical scrollbars should not have children");
			}

			int32 width = Width(panelDimensions);

			topButton.left = bottomButton.left = panelDimensions.left + 1;
			topButton.right = bottomButton.right = panelDimensions.right - 1;
			topButton.top = panelDimensions.top + 1;
			topButton.bottom = topButton.top + width;
			bottomButton.bottom = panelDimensions.bottom - 1;
			bottomButton.top = bottomButton.bottom - width;

			sliderZone.left = panelDimensions.left + 1;
			sliderZone.right = panelDimensions.right - 1;
			sliderZone.top = topButton.bottom + 1;
			sliderZone.bottom = bottomButton.top - 1;
		}

		enum class EClick
		{
			None,
			Top,
			Slider,
			Bar,
			Bottom
		} clickTarget = EClick::None;

		EClick ClassifyTarget(Vec2i pos)
		{
			GuiRect topTarget = topButton;
			GuiRect bottomTarget = bottomButton;
			GuiRect sliderTarget = topButton;
			GuiRect sliderRect = ComputerSliderRect();

			if (IsPointInRect(pos, topTarget))
			{
				return EClick::Top;
			}
			else if (IsPointInRect(pos, sliderRect))
			{
				return EClick::Slider;
			}
			else if (IsPointInRect(pos, bottomTarget))
			{
				return EClick::Bottom;
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

		void AdjustSlider(int dy)
		{

		}

		void MoveLine(int delta)
		{
			scrollPosition += delta;
		}

		void MovePage(int delta)
		{
			scrollPosition += (windowSize * delta * 4) / 5;
		}

		void ActivateTarget(int y)
		{
			switch (clickTarget)
			{
			case EClick::Top:
				MoveLine(-1);
				break;
			case EClick::Bottom:
				MoveLine(1);
				break;
			case EClick::Bar:
				{
					GuiRect sliderRect = ComputerSliderRect();
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
			GuiRect topTarget = topButton + TopLeft(panel.AbsRect());
			GuiRect bottomTarget = bottomButton + TopLeft(panel.AbsRect());
			GuiRect sliderTarget = topButton + TopLeft(panel.AbsRect());
			GuiRect sliderRect = ComputerSliderRect();

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

				int32 height = ComputeSliderHeight();
				if (clickDeltaPosition != 0 && height > 0 && height < Height(sliderZone) - 3)
				{
					int32 dy = clamp(clickDeltaPosition, 0, Height(sliderZone) - height);
					scrollPosition = GetSlidingScrollPosition(dy);
				}
				panel.Root().ReleaseCursor();

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
				clickDeltaPosition = ce.position.y - clickPosition;
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

		GuiRect ComputerSliderRect() const
		{
			int32 height = ComputeSliderHeight();
			if (height <= 0 || height >= Height(sliderZone) - 3)
			{
				return { 0,0,0,0 };
			}

			int32 topPadding = ComputeSliderTopPadding();

			int32 dy = clamp(topPadding + clickDeltaPosition, 0, Height(sliderZone) - height);
			int32 y = sliderZone.top + 1 + dy;
			return { sliderZone.left + 1, y, sliderZone.right - 1, y + height };
		}

		void Render(IGRRenderContext& g) override
		{
			auto rect = panel.AbsRect();

			RenderScrollerButton(g, topButton, true);
			RenderScrollerButton(g, bottomButton, false);

			bool isHovered = g.IsHovered(panel);
			
			RGBAb edge1Colour = panel.GetColour(isHovered ? ESchemeColourSurface::SCROLLER_BAR_TOP_LEFT_HOVERED : ESchemeColourSurface::SCROLLER_BAR_TOP_LEFT);
			RGBAb edge2Colour = panel.GetColour(isHovered ? ESchemeColourSurface::SCROLLER_BAR_BOTTOM_RIGHT_HOVERED : ESchemeColourSurface::SCROLLER_BAR_BOTTOM_RIGHT);
			g.DrawRectEdge(rect, edge1Colour, edge2Colour);

			if (isHovered)
			{
				g.DrawRectEdge(sliderZone, edge1Colour, edge2Colour);
			}

			RenderScrollerSlider(g, ComputerSliderRect());
		}

		int32 ComputeSliderHeight() const
		{
			if (scrollRange == 0 || windowSize == 0)
			{
				return 0;
			}

			int64 pixelRange = Height(sliderZone) - 2;
			return (int32) (pixelRange * (int64)windowSize / (int64)scrollRange);
		}

		int32 ComputeSliderTopPadding() const
		{
			if (scrollRange == 0 || windowSize == 0)
			{
				return 0;
			}

			int64 pixelRange = Height(sliderZone) - 2;
			int64 topPadding = pixelRange * (int64)scrollPosition / (int64)scrollRange;
			return (int32)topPadding;
		}

		int32 GetSlidingScrollPosition(int deltaPadding)
		{
			int64 pixelRange = Height(sliderZone) - 2;

			if (pixelRange <= 0 || windowSize == 0)
			{
				return scrollPosition;
			}

			// p = R * s / S; -> where p is the top padding, R is the pixel range, s is the scroll position and and S is the scroll range
			// =>  S * p / R = s;

			int64 newScrollPosition = (int64)scrollRange * (ComputeSliderTopPadding() + deltaPadding) / pixelRange;
			return (int32) newScrollPosition;
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

		int32 scrollPosition = 0;

		enum { MAX_SCROLL_INT = 1 << 30 };

		void SetPosition(int position) override
		{
			if (position < 0 || position > MAX_SCROLL_INT)
			{
				panel.Root().Custodian().RaiseError(GRErrorCode::InvalidArg, __FUNCTION__, "Position was out of bounds");
				return;
			}
			this->scrollPosition = position;
		}

		int32 scrollRange = 0;

		void SetRange(int range) override
		{
			if (range < 0 || range > MAX_SCROLL_INT)
			{
				panel.Root().Custodian().RaiseError(GRErrorCode::InvalidArg, __FUNCTION__, "Range was out of bounds");
				return;
			}
			this->scrollRange = range;
		}

		int32 windowSize = 0;

		void SetWindowSize(int32 windowSize) override
		{
			if (windowSize < 0 || windowSize > MAX_SCROLL_INT)
			{
				panel.Root().Custodian().RaiseError(GRErrorCode::InvalidArg, __FUNCTION__, "Window size was out of bounds");
				return;
			}
			this->windowSize = windowSize;
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

	ROCOCO_GUI_RETAINED_API IGRWidgetVerticalScroller& CreateVerticalScroller(IGRWidget& parent)
	{
		auto& gr = parent.Panel().Root().GR();
		auto* scroller = Cast<IGRWidgetVerticalScroller>(gr.AddWidget(parent.Panel(), ANON::s_VerticalScrollerFactory));
		return *scroller;
	}
}