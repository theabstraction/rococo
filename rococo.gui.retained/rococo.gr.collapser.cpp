#include <rococo.gui.retained.h>
#include <rococo.maths.i32.h>

using namespace Rococo;
using namespace Rococo::Gui;

namespace GRANON
{
	struct GRCollapser : IGRWidgetCollapser
	{
		IGRPanel& panel;
		IGRWidgetButton* collapseButton = nullptr;
		IGRWidgetDivision* titleBar = nullptr;
		IGRWidgetDivision* clientArea = nullptr;

		GRCollapser(IGRPanel& owningPanel) : panel(owningPanel)
		{
			
		}

		bool IsCollapsed() const
		{
			return collapseButton ? !collapseButton->GetButtonFlags().isRaised : false;
		}

		IGRWidgetDivision& ClientArea() override
		{
			return *clientArea;
		}

		IGRWidgetDivision& TitleBar() override
		{
			return *titleBar;
		}

		void PostConstruct()
		{
			clientArea = &CreateDivision(*this);
			titleBar = &CreateDivision(*this);
			collapseButton = &CreateButton(*titleBar);
			collapseButton->Panel().Resize({ 26,26 }).SetParentOffset({0,0});
			collapseButton->SetRaisedImagePath("$(COLLAPSER_COLLAPSE)");
			collapseButton->SetPressedImagePath("$(COLLAPSER_EXPAND)");
			collapseButton->SetEventPolicy(GREventPolicy::NotifyAncestors);
			collapseButton->MakeToggleButton();
		}

		void Free() override
		{
			delete this;
		}

		void Layout(const GuiRect& panelDimensions) override
		{
			enum { TITLE_BAR_HEIGHT = 30 };
			titleBar->Panel().Resize({Width(panelDimensions), TITLE_BAR_HEIGHT });

			Vec2i newClientSpan;

			if (IsCollapsed())
			{
				newClientSpan = { Width(panelDimensions), 0 };
			}
			else
			{
				newClientSpan = { Width(panelDimensions), Height(panelDimensions) - TITLE_BAR_HEIGHT };
			}

			clientArea->Panel().Resize(newClientSpan);
			clientArea->Panel().SetParentOffset({ 0, TITLE_BAR_HEIGHT });
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
		}

		EventRouting OnChildEvent(WidgetEvent& widgetEvent, IGRWidget& sourceWidget)
		{
			if (sourceWidget.Panel().Id() == collapseButton->Panel().Id())
			{
				panel.InvalidateLayout(true);
				return EventRouting::Terminate;
			}
			return EventRouting::NextHandler;
		}

		EventRouting OnKeyEvent(KeyEvent& keyEvent) override
		{
			return collapseButton->OnKeyEvent(keyEvent);
		}

		Vec2i EvaluateMinimalSpan() const override
		{
			return { 0,0 };
		}

		EQueryInterfaceResult QueryInterface(IGRBase** ppOutputArg, cstr interfaceId) override
		{
			if (!interfaceId || *interfaceId == 0) return EQueryInterfaceResult::INVALID_ID;
			if (DoInterfaceNamesMatch(interfaceId, "IGRWidgetCollapser"))
			{
				*ppOutputArg = this;
				return EQueryInterfaceResult::SUCCESS;
			}

			return EQueryInterfaceResult::NOT_IMPLEMENTED;
		}
	};

	struct GRCollapserFactory : IGRWidgetFactory
	{
		IGRWidget& CreateWidget(IGRPanel& panel)
		{
			return *new GRCollapser(panel);
		}
	} s_CollapserFactory;
}

namespace Rococo::Gui
{
	ROCOCO_GUI_RETAINED_API IGRWidgetCollapser& CreateCollapser(IGRWidget& parent)
	{
		auto& gr = parent.Panel().Root().GR();
		auto& collapser = static_cast<GRANON::GRCollapser&>(gr.AddWidget(parent.Panel(), GRANON::s_CollapserFactory));
		collapser.PostConstruct();
		return collapser;
	}
}