#include <rococo.gui.retained.ex.h>
#include <rococo.maths.i32.h>

using namespace Rococo;
using namespace Rococo::Gui;

namespace GRANON
{
	// Note - this is not that much different from a division widget, and was used for vertical layout before
	// the panels had such layouts embedded in them.
	struct GRVerticalList : IGRWidgetVerticalList, IGRNavigator, IGRWidgetSupervisor
	{
		IGRPanel& panel;
		bool enforcePositiveChildHeights;

		GRVerticalList(IGRPanel& owningPanel, bool _enforcePositiveChildHeights) :
			panel(owningPanel), 
			enforcePositiveChildHeights(_enforcePositiveChildHeights)
		{
			owningPanel.SetMinimalSpan({ 10, 10 });
			owningPanel.SetLayoutDirection(ELayoutDirection::TopToBottom);
		}

		void Free() override
		{
			delete this;
		}

		IGRWidget& Widget() override
		{
			return *this;
		}

		EGREventRouting OnCursorClick(GRCursorEvent& ce) override
		{
			UNUSED(ce);
			return EGREventRouting::NextHandler;
		}

		EGREventRouting OnCursorMove(GRCursorEvent& ce) override
		{
			UNUSED(ce);
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

			// g.DrawRectEdge(panel.AbsRect(), RGBAb(255, 0, 0, 255), RGBAb(255, 0, 0, 255));
		}

		EGREventRouting OnChildEvent(GRWidgetEvent&, IGRWidget&) override
		{
			return EGREventRouting::NextHandler;
		}

		EGREventRouting OnTab()
		{
			auto focusId = panel.Root().GR().GetFocusId();
			auto* focus = panel.Root().GR().FindWidget(focusId);
			if (!focus)
			{
				return EGREventRouting::Terminate;
			}

			int32 childIndex = 0;
			while (auto* child = panel.GetChild(childIndex++))
			{
				if (IsCandidateDescendantOfParent(*child, focus->Panel()))
				{
					auto* nextChild = panel.GetChild(childIndex++);
					if (nextChild == nullptr && panel.HasFlag(EGRPanelFlags::CycleTabsEndlessly))
					{
						nextChild = panel.GetChild(0);
					}
					
					if (nextChild && TrySetDeepFocus(*nextChild))
					{
						return EGREventRouting::Terminate;
					}
				}
			}

			return EGREventRouting::NextHandler;
		}

		EGREventRouting OnNavigate(EGRNavigationDirective directive) override
		{
			if (directive == EGRNavigationDirective::Tab)
			{
				return OnTab();
			}

			return EGREventRouting::NextHandler;
		}

		EGRQueryInterfaceResult QueryInterface(IGRBase** ppOutputArg, cstr interfaceId) override
		{
			if (ppOutputArg) *ppOutputArg = nullptr;
			if (!interfaceId || *interfaceId == 0) return EGRQueryInterfaceResult::INVALID_ID;

			if (DoInterfaceNamesMatch(interfaceId, IGRWidgetVerticalList::InterfaceId()))
			{
				if (ppOutputArg)
				{
					*ppOutputArg = static_cast<IGRWidgetVerticalList*>(this);
				}

				return EGRQueryInterfaceResult::SUCCESS;
			}
			else if (DoInterfaceNamesMatch(interfaceId, IGRNavigator::InterfaceId()))
			{
				if (ppOutputArg)
				{
					*ppOutputArg = static_cast<IGRNavigator*>(this);
				}

				return EGRQueryInterfaceResult::SUCCESS;
			}

			return EGRQueryInterfaceResult::NOT_IMPLEMENTED;
		}

		cstr GetImplementationTypeName() const override
		{
			return "GRVerticalList";
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
		auto* list = Cast<IGRWidgetVerticalList>(gr.AddWidget(parent.Panel(), factory));
		return *list;
	}
}