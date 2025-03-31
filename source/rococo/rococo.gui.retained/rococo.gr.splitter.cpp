#include <rococo.gui.retained.ex.h>
#include <rococo.maths.i32.h>
#include <rococo.impressario.inl>
#include <vector>

namespace GRANON
{
	using namespace Rococo;
	using namespace Rococo::Gui;

	struct GRSplitter : IGRWidgetSplitter, IGRWidgetSupervisor
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
			_panel.SetMinimalSpan({ 320, 200 });
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

			GRRenderState rs(false, isHovered, false);

			RGBAb colour = panel.GetColour(EGRSchemeColourSurface::SPLITTER_BACKGROUND, rs, RGBAb(64, 64, 64, 255));
			g.DrawRect(draggerRect, colour);

			RGBAb edgeColour = panel.GetColour(EGRSchemeColourSurface::SPLITTER_EDGE, rs, RGBAb(64, 64, 64, 255));
			g.DrawRectEdge(draggerRect, edgeColour, edgeColour);

			if (virtualDraggerStartPos >= 0)
			{
				int delta = g.CursorHoverPoint().x - virtualDraggerStartPos;
				int virtualDraggerPos = clamp(clamp(draggerStartPos + delta, 0, panel.Span().x - draggerThickness - 2), splitterMin, splitterMax);

				GuiRect virtualRect = panel.AbsRect();
				virtualRect.left += virtualDraggerPos + 1;
				virtualRect.right = virtualRect.left + draggerThickness - 2;

				GRRenderState litEdgeState(true, isHovered, false);
				RGBAb litEdge = panel.GetColour(EGRSchemeColourSurface::SPLITTER_EDGE, litEdgeState, RGBAb(255, 255, 255, 255));
				g.DrawRectEdgeLast(virtualRect, litEdge, litEdge);
			}
		}

		void Free() override
		{
			delete this;
		}

		void LayoutHorizontal(const GuiRect& panelDimensions)
		{
			realDraggerStartPos = min(realDraggerStartPos, Width(panelDimensions) - draggerThickness);
			first->Panel().SetParentOffset({ 0,0 }).Resize({ realDraggerStartPos, Height(panelDimensions)});
			second->Panel().SetParentOffset({ realDraggerStartPos + draggerThickness,0 }).Resize({ Width(panelDimensions) - (realDraggerStartPos + draggerThickness), Height(panelDimensions)});
		}

		EGREventRouting OnCursorClick(GRCursorEvent& ce) override
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
					draggerStartPos = realDraggerStartPos;

					evOnSplitterChanged.Invoke(realDraggerStartPos);
				}

				return EGREventRouting::Terminate;
			}
			else if (ce.click.LeftButtonDown)
			{
				if (virtualDraggerStartPos > 0)
				{
					// We lost focus and missed the previous mouse down
					virtualDraggerStartPos = -1;
					draggerStartPos = realDraggerStartPos;
					return EGREventRouting::Terminate;;
				}
				if (IsPointInRect(ce.position, draggerRect))
				{
					panel.Focus();
					virtualDraggerStartPos = ce.position.x;
					return EGREventRouting::Terminate;
				}
			}

			return EGREventRouting::NextHandler;
		}

		EGREventRouting OnChildEvent(GRWidgetEvent&, IGRWidget&) override
		{
			return EGREventRouting::NextHandler;
		}

		EGREventRouting OnCursorMove(GRCursorEvent& ce) override
		{
			if (!panel.HasFocus())
			{
				if (virtualDraggerStartPos >= 0)
				{
					virtualDraggerStartPos = -1;
					draggerStartPos = realDraggerStartPos;
				}
			}

			bool isDraggerHovered = IsPointInRect(ce.position, draggerRect);
			if (isDraggerHovered)
			{
				ce.nextIcon = EGRCursorIcon::LeftAndRightDragger;
			}

			if (updateWithMouseMove && virtualDraggerStartPos >= 0)
			{
				int delta = ce.position.x - virtualDraggerStartPos;
				realDraggerStartPos = clamp(clamp(draggerStartPos + delta, 0, panel.Span().x - draggerThickness - 2), splitterMin, splitterMax);;
			}

			return EGREventRouting::NextHandler;
		}

		void OnCursorEnter() override
		{

		}

		void OnCursorLeave() override
		{

		}

		EGREventRouting OnKeyEvent(GRKeyEvent&) override
		{
			return EGREventRouting::NextHandler;
		}

		IGRPanel& Panel() override
		{
			return panel;
		}

		IGRWidgetDivision& First() override
		{
			return *first;
		}

		IGRWidgetDivision& Second() override
		{
			return *second;
		}

		EGRQueryInterfaceResult QueryInterface(IGRBase** ppOutputArg, cstr interfaceId) override
		{
			return Gui::QueryForParticularInterface<IGRWidgetSplitter>(this, ppOutputArg, interfaceId);
		}

		IGRWidgetSplitter& SetDraggerMinMax(int32 minValue, int32 maxValue) override
		{
			splitterMin = minValue;
			splitterMax = maxValue;
			return *this;
		}

		EventImpressario<int> evOnSplitterChanged;

		IEventImpressario<int>& EvOnSplitSizeChanged()
		{
			return evOnSplitterChanged;
		}

		IGRWidget& Widget() override
		{
			return *this;
		}

		cstr GetImplementationTypeName() const override
		{
			return "GRSplitter";
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
	ROCOCO_GUI_RETAINED_API cstr IGRWidgetSplitter::InterfaceId()
	{
		return "IGRWidgetSplitter";
	}

	ROCOCO_GUI_RETAINED_API IGRWidgetSplitter& CreateLeftToRightSplitter(IGRWidget& parent, int32 draggerStartPos, bool updateWithMouseMove)
	{
		auto& gr = parent.Panel().Root().GR();

		GRANON::GRSplitterFactory factory;
		factory.draggerStartPos = draggerStartPos;
		factory.updateWithMouseMove = updateWithMouseMove;
		return *Cast<IGRWidgetSplitter>(gr.AddWidget(parent.Panel(), factory));
	}
}