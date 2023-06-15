#include <rococo.gui.retained.ex.h>
#include <rococo.maths.i32.h>

using namespace Rococo;
using namespace Rococo::Gui;

namespace GRANON
{
	struct GRVerticalList : IGRWidgetVerticalList
	{
		IGRPanel& panel;
		bool enforcePositiveChildHeights;

		GRVerticalList(IGRPanel& owningPanel, bool _enforcePositiveChildHeights) :
			panel(owningPanel), 
			enforcePositiveChildHeights(_enforcePositiveChildHeights)
		{
			owningPanel.SetMinimalSpan({ 10, 10 });
		}

		void Free() override
		{
			delete this;
		}

		void Layout(const GuiRect& panelDimensions) override
		{
			int index = 0;
			int top = 0;

			while (auto* child = panel.GetChild(index++))
			{
				auto padding = child->Padding();
				child->SetParentOffset({ padding.left, top + padding.top });
				int dy = child->Span().y;
				if (dy <= 0 && enforcePositiveChildHeights)
				{
					panel.Root().Custodian().RaiseError(GRErrorCode::Generic, __FUNCTION__, "Child of vertical list had zero height");
				}
				child->Resize({Width(panelDimensions) - padding.left - padding.right, dy});
				top += dy + padding.top + padding.bottom;
			}

			int overhang = top - Height(panelDimensions);
			if (overhang > 0)
			{
				// We need vertical scrolling
			}
		}

		EventRouting OnCursorClick(CursorEvent& ce) override
		{
			return EventRouting::NextHandler;
		}

		EventRouting OnCursorMove(CursorEvent& ce) override
		{
			return EventRouting::NextHandler;
		}

		void OnCursorEnter() override
		{

		}

		void OnCursorLeave() override
		{

		}

		EventRouting OnKeyEvent(KeyEvent& keyEvent) override
		{
			return EventRouting::NextHandler;
		}

		IGRPanel& Panel() override
		{
			return panel;
		}

		void Render(IGRRenderContext& g) override
		{
			auto rect = panel.AbsRect();

			bool isHovered = g.IsHovered(panel);

			RGBAb backColour = panel.GetColour(isHovered ? ESchemeColourSurface::CONTAINER_BACKGROUND_HOVERED : ESchemeColourSurface::CONTAINER_BACKGROUND);
			g.DrawRect(rect, backColour);

			RGBAb edge1Colour = panel.GetColour(isHovered ? ESchemeColourSurface::CONTAINER_TOP_LEFT_HOVERED : ESchemeColourSurface::CONTAINER_TOP_LEFT);
			RGBAb edge2Colour = panel.GetColour(isHovered ? ESchemeColourSurface::CONTAINER_BOTTOM_RIGHT_HOVERED : ESchemeColourSurface::CONTAINER_BOTTOM_RIGHT);
			g.DrawRectEdge(rect, edge1Colour, edge2Colour);

			// g.DrawRectEdge(panel.AbsRect(), RGBAb(255, 0, 0, 255), RGBAb(255, 0, 0, 255));
		}

		EventRouting OnChildEvent(WidgetEvent& widgetEvent, IGRWidget& sourceWidget) override
		{
			return EventRouting::NextHandler;
		}

		EQueryInterfaceResult QueryInterface(IGRBase** ppOutputArg, cstr interfaceId) override
		{
			return QueryForParticularInterface<IGRWidgetVerticalList>(this, ppOutputArg, interfaceId);
		}
	};

	struct GRVerticalListFactory : IGRWidgetFactory
	{
		bool enforcePositiveChildHeights;

		GRVerticalListFactory(bool _enforcePositiveChildHeights): enforcePositiveChildHeights(_enforcePositiveChildHeights)
		{

		}

		IGRWidget& CreateWidget(IGRPanel& panel)
		{
			return *new GRVerticalList(panel, enforcePositiveChildHeights);
		}
	};
}

namespace Rococo::Gui
{
	ROCOCO_GUI_RETAINED_API cstr IGRWidgetVerticalList::InterfaceId()
	{
		return "IGRWidgetVerticalList";
	}

	ROCOCO_GUI_RETAINED_API IGRWidgetVerticalList& CreateVerticalList(IGRWidget& parent, bool enforcePositiveChildHeights)
	{
		GRANON::GRVerticalListFactory factory(enforcePositiveChildHeights);
		auto& gr = parent.Panel().Root().GR();
		auto& list = static_cast<IGRWidgetVerticalList&>(gr.AddWidget(parent.Panel(), factory));
		return list;
	}
}