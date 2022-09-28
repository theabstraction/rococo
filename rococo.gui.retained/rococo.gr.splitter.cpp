#include <rococo.gui.retained.h>
#include <rococo.maths.i32.h>

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
		GuiRect draggerRect {0,0,0,0};
		int32 virtualDraggerStartPos = -1;
		bool updateWithMouseMove;
		int32 splitterMin = 0;
		int32 splitterMax = 8192;

		GRSplitter(IGRPanel& _panel, int32 _draggerStartPos, bool _updateWithMouseMove) : panel(_panel), draggerStartPos(_draggerStartPos), updateWithMouseMove(_updateWithMouseMove)
		{
			realDraggerStartPos = _draggerStartPos;
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
			draggerRect = panel.AbsRect();
			draggerRect.left += realDraggerStartPos + 1;
			draggerRect.right = draggerRect.left + draggerThickness - 2;

			bool isHovered = IsPointInRect(g.CursorHoverPoint(), draggerRect);
			RGBAb colour = isHovered ? panel.GetColour(ESchemeColourSurface::SPLITTER_BACKGROUND_HOVERED, RGBAb(128, 128, 128, 255)) : panel.GetColour(ESchemeColourSurface::SPLITTER_BACKGROUND, RGBAb(64, 64, 64, 255));
			g.DrawRect(draggerRect, colour);

			RGBAb edgeColour = isHovered ? panel.GetColour(ESchemeColourSurface::SPLITTER_EDGE_HILIGHTED, RGBAb(255, 255, 255, 255)) : panel.GetColour(ESchemeColourSurface::SPLITTER_EDGE_HILIGHTED, RGBAb(64, 64, 64, 255));
			g.DrawRectEdge(draggerRect, edgeColour, edgeColour);

			if (virtualDraggerStartPos >= 0)
			{
				int delta = g.CursorHoverPoint().x - virtualDraggerStartPos;
				int virtualDraggerPos = clamp(clamp(draggerStartPos + delta, 0, panel.Span().x - draggerThickness - 2), splitterMin, splitterMax);

				GuiRect virtualRect = panel.AbsRect();
				virtualRect.left += virtualDraggerPos + 1;
				virtualRect.right = virtualRect.left + draggerThickness - 2;

				RGBAb litEdge = panel.GetColour(ESchemeColourSurface::SPLITTER_EDGE_HILIGHTED, RGBAb(255, 255, 255, 255));
				g.DrawRectEdgeLast(virtualRect, litEdge, litEdge);
			}
		}

		void Free() override
		{
			delete this;
		}

		void LayoutHorizontal(const GuiRect& screenDimensions)
		{
			realDraggerStartPos = min(realDraggerStartPos, Width(screenDimensions) - draggerThickness);
			first->Panel().SetParentOffset({ 0,0 }).Resize({ realDraggerStartPos, Height(screenDimensions)});
			second->Panel().SetParentOffset({ realDraggerStartPos + draggerThickness,0 }).Resize({ Width(screenDimensions) - (realDraggerStartPos + draggerThickness), Height(screenDimensions)});
		}

		void Layout(const GuiRect& screenDimensions) override
		{
			LayoutHorizontal(screenDimensions);
		}

		EventRouting OnCursorClick(CursorEvent& ce) override
		{
			if (ce.click.LeftButtonUp)
			{
				if (panel.Id() == panel.Root().GR().GetFocusId())
				{
					panel.Root().GR().SetFocus(-1);
				}

				if (virtualDraggerStartPos >= 0)
				{
					int delta = ce.position.x - virtualDraggerStartPos;
					virtualDraggerStartPos = -1;
					realDraggerStartPos = clamp(clamp(draggerStartPos + delta, 0, panel.Span().x - draggerThickness - 2), splitterMin, splitterMax);
					panel.InvalidateLayout(true);
					draggerStartPos = realDraggerStartPos;
				}

				return EventRouting::Terminate;
			}
			else if (ce.click.LeftButtonDown)
			{
				if (IsPointInRect(ce.position, draggerRect))
				{
					panel.Focus();
					virtualDraggerStartPos = ce.position.x;
					return EventRouting::Terminate;
				}
			}

			return EventRouting::NextHandler;
		}

		EventRouting OnChildEvent(WidgetEvent& widgetEvent, IGRWidget& sourceWidget) override
		{
			return EventRouting::NextHandler;
		}

		EventRouting OnCursorMove(CursorEvent& ce) override
		{
			bool isDraggerHovered = IsPointInRect(ce.position, draggerRect);
			if (isDraggerHovered)
			{
				ce.nextIcon = ECursorIcon::LeftAndRightDragger;
			}

			if (updateWithMouseMove && virtualDraggerStartPos >= 0)
			{
				int delta = ce.position.x - virtualDraggerStartPos;
				realDraggerStartPos = clamp(clamp(draggerStartPos + delta, 0, panel.Span().x - draggerThickness - 2), splitterMin, splitterMax);
				panel.InvalidateLayout(true);
			}

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

		EQueryInterfaceResult QueryInterface(IGRBase** ppOutputArg, cstr interfaceId) override
		{			
			if (!interfaceId || *interfaceId == 0) return EQueryInterfaceResult::INVALID_ID;
			if (DoInterfaceNamesMatch(interfaceId, "IGRWidgetSplitter"))
			{
				if (ppOutputArg) *ppOutputArg = this;
				return EQueryInterfaceResult::SUCCESS;
			}

			return EQueryInterfaceResult::NOT_IMPLEMENTED;
		}

		IGRWidgetSplitter& SetDraggerMinMax(int32 minValue, int32 maxValue) override
		{
			splitterMin = minValue;
			splitterMax = maxValue;
			return *this;
		}
	};

	struct GRSplitterFactory : IGRWidgetFactory
	{
		int32 draggerStartPos;
		bool updateWithMouseMove;

		IGRWidget& CreateWidget(IGRPanel& panel)
		{
			auto splitter = new GRSplitter(panel, draggerStartPos, updateWithMouseMove);
			splitter->PostConstruct();
			return *splitter;
		}
	};
}

namespace Rococo::Gui
{
	ROCOCO_GUI_RETAINED_API IGRWidgetSplitter& CreateLeftToRightSplitter(IGRWidget& parent, int32 draggerStartPos, bool updateWithMouseMove)
	{
		auto& gr = parent.Panel().Root().GR();

		GRANON::GRSplitterFactory factory;
		factory.draggerStartPos = draggerStartPos;
		factory.updateWithMouseMove = updateWithMouseMove;
		return static_cast<IGRWidgetSplitter&>(gr.AddWidget(parent.Panel(), factory));
	}
}