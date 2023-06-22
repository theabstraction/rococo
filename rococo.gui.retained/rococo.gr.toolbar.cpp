#include <rococo.gui.retained.ex.h>
#include <rococo.maths.i32.h>
#include <string>

using namespace Rococo;
using namespace Rococo::Gui;

namespace GRANON
{
	struct GRToolbar : IGRWidgetToolbar, IGRWidget
	{
		IGRPanel& panel;
		EGRAlignment childAlignment = EGRAlignment::Left;
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
				child->Resize(child->MinimalSpan());
				child->InvalidateLayout(false);
			}

			if (childAlignment == EGRAlignment::Left)
			{
				LayoutWithLeftAlignment(panelDimensions);
			}
			else
			{
				LayoutWithRightAlignment(panelDimensions);
			}
		}

		EGREventRouting OnCursorClick(GRCursorEvent& ce) override
		{
			return EGREventRouting::NextHandler;
		}

		EGREventRouting OnCursorMove(GRCursorEvent& ce) override
		{
			return EGREventRouting::NextHandler;
		}

		void OnCursorEnter() override
		{

		}

		void OnCursorLeave() override
		{

		}

		IGRPanel& Panel() override
		{
			return panel;
		}

		void Render(IGRRenderContext& g) override
		{
			auto rect = panel.AbsRect();

			bool isHovered = g.IsHovered(panel);
			GRRenderState rs(false, isHovered, false);

			RGBAb backColour = panel.GetColour(EGRSchemeColourSurface::CONTAINER_BACKGROUND, rs);
			g.DrawRect(rect, backColour);

			RGBAb edge1Colour = panel.GetColour(EGRSchemeColourSurface::CONTAINER_TOP_LEFT, rs);
			RGBAb edge2Colour = panel.GetColour(EGRSchemeColourSurface::CONTAINER_BOTTOM_RIGHT, rs);
			g.DrawRectEdge(rect, edge1Colour, edge2Colour);
		}

		Vec2i ResizeToFitChildren() override
		{
			int32 dx = 0;

			int32 index = 0;
			while (auto* child = panel.GetChild(index++))
			{
				auto newSpan = child->MinimalSpan();
				child->Resize(newSpan);
				dx += newSpan.x + interChildPadding;
			}

			panel.Resize({dx - interChildPadding, panel.Span().y});
			return panel.Span();
		}

		void SetChildAlignment(EGRAlignment alignment, int32 interChildPadding, int32 borderPadding) override
		{
			this->childAlignment = alignment;
			this->interChildPadding = interChildPadding;
			this->borderPadding = borderPadding;
		}

		EGREventRouting OnChildEvent(GRWidgetEvent& widgetEvent, IGRWidget& sourceWidget)
		{
			return EGREventRouting::NextHandler;
		}

		EGREventRouting OnKeyEvent(GRKeyEvent& keyEvent) override
		{
			return EGREventRouting::NextHandler;
		}

		EGRQueryInterfaceResult QueryInterface(IGRBase** ppOutputArg, cstr interfaceId) override
		{
			return Gui::QueryForParticularInterface<IGRWidgetToolbar>(this, ppOutputArg, interfaceId);
		}

		IGRWidget& Widget() override
		{
			return *this;
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
	ROCOCO_GUI_RETAINED_API cstr IGRWidgetToolbar::InterfaceId()
	{
		return "IGRWidgetToolbar";
	}

	ROCOCO_GUI_RETAINED_API IGRWidgetToolbar& CreateToolbar(IGRWidget& parent)
	{
		auto& gr = parent.Panel().Root().GR();
		auto* tools = Cast<IGRWidgetToolbar>(gr.AddWidget(parent.Panel(), GRANON::s_ToolbarFactory));
		return* tools;
	}
}