#include <rococo.gui.retained.h>
#include <rococo.maths.h>

namespace GRANON
{
	using namespace Rococo;
	using namespace Rococo::Gui;

	struct GRSplitter : IGRWidgetSplitter
	{
		IGRPanel& panel;
		IGRWidgetDivision* first = nullptr;
		IGRWidgetDivision* second = nullptr;
		int32 draggerThickness = 8;
		int32 draggerStartPos = 0;
		int32 realDraggerStartPos = 0;
		bool isHorizontal = true;

		GRSplitter(IGRPanel& _panel, int32 _draggerStartPos) : panel(_panel), draggerStartPos(_draggerStartPos)
		{

		}

		void PostConstruct()
		{
			if (!first)
			{
				first = &CreateDivision(*this);
				second = &CreateDivision(*this);
			}
		}

		void Render(IGRRenderContext& g)
		{
			GuiRect rect = panel.AbsRect();
			rect.left += realDraggerStartPos + 1;
			rect.right = rect.left + draggerThickness - 2;

			bool isHovered = IsPointInRect(g.CursorHoverPoint(), rect);
			RGBAb colour = isHovered ? panel.GetColour(ESchemeColourSurface::SPLITTER_BACKGROUND_HOVERED, RGBAb(128, 128, 128, 255)) : panel.GetColour(ESchemeColourSurface::SPLITTER_BACKGROUND, RGBAb(64, 64, 64, 255));
			g.DrawRect(rect, colour);

			RGBAb edgeColour = isHovered ? panel.GetColour(ESchemeColourSurface::SPLITTER_EDGE_HILIGHTED, RGBAb(255, 255, 255, 255)) : panel.GetColour(ESchemeColourSurface::SPLITTER_EDGE_HILIGHTED, RGBAb(64, 64, 64, 255));
			g.DrawRectEdge(rect, edgeColour, edgeColour);
		}

		void Free() override
		{
			delete this;
		}

		void LayoutHorizontal(const GuiRect& screenDimensions)
		{
			realDraggerStartPos = min(draggerStartPos, Width(screenDimensions) - draggerThickness);
			first->Panel().SetParentOffset({ 0,0 }).Resize({ realDraggerStartPos, Height(screenDimensions)});
			second->Panel().SetParentOffset({ realDraggerStartPos + draggerThickness,0 }).Resize({ Width(screenDimensions) - (realDraggerStartPos + draggerThickness), Height(screenDimensions)});
		}

		void Layout(const GuiRect& screenDimensions) override
		{
			LayoutHorizontal(screenDimensions);
		}

		EventRouting OnCursorClick(CursorEvent& ce) override
		{
			return EventRouting::NextHandler;
		}

		EventRouting OnChildEvent(WidgetEvent& widgetEvent, IGRWidget& sourceWidget) override
		{
			return EventRouting::NextHandler;
		}

		EventRouting OnCursorMove(CursorEvent& ce) override
		{
			return EventRouting::NextHandler;
		}

		EventRouting OnKeyEvent(KeyEvent& keyEvent) override
		{
			return EventRouting::NextHandler;
		}

		IGRPanel& Panel() override
		{
			return panel;
		}

		Vec2i EvaluateMinimalSpan() const override
		{
			return Vec2i{ 320, 200 };
		}

		IGRWidgetDivision& First() override
		{
			return *first;
		}

		IGRWidgetDivision& Second() override
		{
			return *second;
		}
	};

	struct GRSplitterFactory : IGRWidgetFactory
	{
		int32 draggerStartPos;

		IGRWidget& CreateWidget(IGRPanel& panel)
		{
			auto splitter = new GRSplitter(panel, draggerStartPos);
			splitter->PostConstruct();
			return *splitter;
		}
	};
}

namespace Rococo::Gui
{
	ROCOCO_GUI_RETAINED_API IGRWidgetSplitter& CreateLeftToRightSplitter(IGRWidget& parent, int32 draggerStartPos)
	{
		auto& gr = parent.Panel().Root().GR();

		GRANON::GRSplitterFactory factory;
		factory.draggerStartPos = draggerStartPos;
		return static_cast<IGRWidgetSplitter&>(gr.AddWidget(parent.Panel(), factory));
	}
}