#include <rococo.gui.retained.h>
#include <rococo.maths.h>

using namespace Rococo;
using namespace Rococo::Gui;

namespace GRANON
{
	struct GRVerticalList : IGRWidgetVerticalList
	{
		IGRPanel& panel;

		GRVerticalList(IGRPanel& owningPanel) : panel(owningPanel)
		{
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


		}

		EventRouting OnChildEvent(WidgetEvent& widgetEvent, IGRWidget& sourceWidget) override
		{
			return EventRouting::NextHandler;
		}

		Vec2i EvaluateMinimalSpan() const override
		{
			return { 0,0 };
		}
	};

	struct GRVerticalListFactory : IGRWidgetFactory
	{
		IGRWidget& CreateWidget(IGRPanel& panel)
		{
			return *new GRVerticalList(panel);
		}
	} s_VerticalListFactory;
}

namespace Rococo::Gui
{
	ROCOCO_GUI_RETAINED_API IGRWidgetVerticalList& CreateVerticalList(IGRWidget& parent)
	{
		auto& gr = parent.Panel().Root().GR();
		auto& list = static_cast<IGRWidgetVerticalList&>(gr.AddWidget(parent.Panel(), GRANON::s_VerticalListFactory));
		return list;
	}
}