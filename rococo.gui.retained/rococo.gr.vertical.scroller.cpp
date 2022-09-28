#include <rococo.gui.retained.h>
#include <rococo.maths.i32.h>

using namespace Rococo;
using namespace Rococo::Gui;

namespace ANON
{
	struct GRVerticalScroller : IGRWidgetVerticalScroller
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
			topButton.top = panelDimensions.top;
			topButton.bottom = topButton.top + width;
			bottomButton.bottom = panelDimensions.bottom;
			bottomButton.top = bottomButton.bottom - width;

			sliderZone.left = panelDimensions.left + 1;
			sliderZone.right = panelDimensions.right - 1;
			sliderZone.top = topButton.bottom + 1;
			sliderZone.bottom = bottomButton.top - 1;
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

		void RenderScrollerButton(IGRRenderContext& g, const GuiRect& rect)
		{
			bool isHovered = IsPointInRect(g.CursorHoverPoint(), rect);

			RGBAb backColour = panel.Root().Scheme().GetColour(isHovered ? ESchemeColourSurface::SCROLLER_BUTTON_BACKGROUND_HOVERED : ESchemeColourSurface::SCROLLER_BUTTON_BACKGROUND);
			g.DrawRect(rect, backColour);

			RGBAb edge1Colour = panel.Root().Scheme().GetColour(isHovered ? ESchemeColourSurface::SCROLLER_BUTTON_TOP_LEFT_HOVERED : ESchemeColourSurface::SCROLLER_BUTTON_TOP_LEFT);
			RGBAb edge2Colour = panel.Root().Scheme().GetColour(isHovered ? ESchemeColourSurface::SCROLLER_BUTTON_BOTTOM_RIGHT_HOVERED : ESchemeColourSurface::SCROLLER_BUTTON_BOTTOM_RIGHT);
			g.DrawRectEdge(rect, edge1Colour, edge2Colour);
		}

		void Render(IGRRenderContext& g) override
		{
			auto rect = panel.AbsRect();

			RenderScrollerButton(g, topButton);
			RenderScrollerButton(g, bottomButton);

			bool isHovered = g.IsHovered(panel);
			
			RGBAb edge1Colour = panel.Root().Scheme().GetColour(isHovered ? ESchemeColourSurface::SCROLLER_BUTTON_TOP_LEFT_HOVERED : ESchemeColourSurface::SCROLLER_BUTTON_TOP_LEFT);
			RGBAb edge2Colour = panel.Root().Scheme().GetColour(isHovered ? ESchemeColourSurface::SCROLLER_BUTTON_BOTTOM_RIGHT_HOVERED : ESchemeColourSurface::SCROLLER_BUTTON_BOTTOM_RIGHT);
			g.DrawRectEdge(rect, edge1Colour, edge2Colour);

			if (isHovered)
			{
				g.DrawRectEdge(sliderZone, edge1Colour, edge2Colour);
			}

			int32 height = ComputeSliderHeight();
			if (height <= 0 || height >= Height(sliderZone) - 3)
			{
				return;
			}

			int32 topPadding = ComputeSliderTopPadding();

			int32 y = sliderZone.top + 1 + topPadding;
			GuiRect sliderRect{ sliderZone.left + 1, y, sliderZone.right - 1, y + height };
			RenderScrollerButton(g, sliderRect);
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

		int32 scrollPosition = 0;

		void SetPosition(int position) override
		{
			if (position < 0 || position >(1 >> 30))
			{
				panel.Root().Custodian().RaiseError(GRErrorCode::InvalidArg, __FUNCTION__, "Position was out of bounds");
				return;
			}
			this->scrollPosition = position;
		}

		int32 scrollRange = 0;

		void SetRange(int range) override
		{
			if (range < 0 || range >(1 >> 30))
			{
				panel.Root().Custodian().RaiseError(GRErrorCode::InvalidArg, __FUNCTION__, "Range was out of bounds");
				return;
			}
			this->scrollRange = range;
		}

		int32 windowSize = 0;

		void SetWindowSize(int32 windowSize) override
		{
			if (windowSize < 0 || windowSize >(1 >> 30))
			{
				panel.Root().Custodian().RaiseError(GRErrorCode::InvalidArg, __FUNCTION__, "Window size was out of bounds");
				return;
			}
			this->windowSize = windowSize;
		}

		EQueryInterfaceResult QueryInterface(IGRBase** ppOutputArg, cstr interfaceId) override
		{
			if (!interfaceId || *interfaceId == 0) return EQueryInterfaceResult::INVALID_ID;
			if (DoInterfaceNamesMatch(interfaceId, "IGRWidgetVerticalScroller"))
			{
				if (ppOutputArg) *ppOutputArg = this;
				return EQueryInterfaceResult::SUCCESS;
			}

			return EQueryInterfaceResult::NOT_IMPLEMENTED;
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
	ROCOCO_GUI_RETAINED_API IGRWidgetVerticalScroller& CreateVerticalScroller(IGRWidget& parent)
	{
		auto& gr = parent.Panel().Root().GR();
		auto& scroller = static_cast<IGRWidgetVerticalScroller&>(gr.AddWidget(parent.Panel(), ANON::s_VerticalScrollerFactory));
		return scroller;
	}
}