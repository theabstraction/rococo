#include <rococo.gui.retained.h>
#include <rococo.maths.i32.h>
#include <string>

using namespace Rococo;
using namespace Rococo::Gui;

namespace GRANON
{
	struct GRToolbar : IGRWidgetToolbar
	{
		IGRPanel& panel;
		GRAlignment childAlignment = GRAlignment::Left;
		int32 interChildPadding = 4;
		int32 borderPadding = 1;

		GRToolbar(IGRPanel& owningPanel) : panel(owningPanel)
		{
		}

		void Free() override
		{
			delete this;
		}

		void LayoutWithLeftAlignment(const GuiRect& panelDimensions)
		{
			int x = 1;

			int32 index = 0;
			while (auto* child = panel.GetChild(index++))
			{
				child->Resize({ child->Span().x, Height(panelDimensions) - 2 });
				x += child->Span().x;
				child->SetParentOffset({ x, 1 });
				x += interChildPadding;
			}
		}

		void LayoutWithRightAlignment(const GuiRect& panelDimensions)
		{
			int32 nChildren = panel.EnumerateChildren(nullptr);
			if (nChildren == 0)
			{
				return;
			}

			int x = Width(panelDimensions) - 1;

			for (int32 i = nChildren - 1; i >= 0; i--)
			{
				auto* child = panel.GetChild(i);
				child->Resize({ child->Span().x, Height(panelDimensions) - 2 });
				x -= child->Span().x;
				child->SetParentOffset({ x, 1 });
				x -= interChildPadding;
			}
		}

		void Layout(const GuiRect& panelDimensions)
		{
			int32 index = 0;
			while (auto* child = panel.GetChild(index++))
			{
				child->Resize(child->Widget().EvaluateMinimalSpan());
				child->InvalidateLayout(false);
			}

			if (childAlignment == GRAlignment::Left)
			{
				LayoutWithLeftAlignment(panelDimensions);
			}
			else
			{
				LayoutWithRightAlignment(panelDimensions);
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

		Vec2i ResizeToFitChildren() override
		{
			int32 dx = 0;

			int32 index = 0;
			while (auto* child = panel.GetChild(index++))
			{
				auto newSpan = child->Widget().EvaluateMinimalSpan();
				child->Resize(newSpan);
				dx += newSpan.x;
			}

			panel.Resize({dx, panel.Span().y});
			return panel.Span();
		}

		void SetChildAlignment(GRAlignment alignment, int32 interChildPadding, int32 borderPadding) override
		{
			this->childAlignment = alignment;
			this->interChildPadding = interChildPadding;
			this->borderPadding = borderPadding;
		}

		EventRouting OnChildEvent(WidgetEvent& widgetEvent, IGRWidget& sourceWidget)
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

		EQueryInterfaceResult QueryInterface(IGRBase** ppOutputArg, cstr interfaceId) override
		{
			if (!interfaceId || *interfaceId == 0) return EQueryInterfaceResult::INVALID_ID;
			if (DoInterfaceNamesMatch(interfaceId, "IGRWidgetToolbar"))
			{
				if (ppOutputArg) *ppOutputArg = this;
				return EQueryInterfaceResult::SUCCESS;
			}

			return EQueryInterfaceResult::NOT_IMPLEMENTED;
		}
	};

	struct GRToolbarFactory : IGRWidgetFactory
	{
		IGRWidget& CreateWidget(IGRPanel& panel)
		{
			return *new GRToolbar(panel);
		}
	} s_ToolbarFactory;
}

namespace Rococo::Gui
{
	ROCOCO_GUI_RETAINED_API IGRWidgetToolbar& CreateToolbar(IGRWidget& parent)
	{
		auto& gr = parent.Panel().Root().GR();
		auto& tools = static_cast<IGRWidgetToolbar&>(gr.AddWidget(parent.Panel(), GRANON::s_ToolbarFactory));
		return tools;
	}
}