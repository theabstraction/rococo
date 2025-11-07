// Copyright (c)2025 Mark Anthony Taylor. Email: mark.anthony.taylor@gmail.com. All rights reserved.
#include <rococo.gui.retained.ex.h>
#include <rococo.maths.i32.h>


#include <rococo.strings.h>

#include <rococo.ui.h>
#include <rococo.vkeys.h>

using namespace Rococo;
using namespace Rococo::Gui;
using namespace Rococo::Strings;

namespace GRANON
{
	static const char* const defaultExpandPath = "$(COLLAPSER_EXPAND)";
	static const char* const defaultInlinePath = "$(COLLAPSER_COLLAPSE)";

	enum { TITLE_BAR_HEIGHT = 30 };

	struct GRCollapser : IGRWidgetCollapser, IGRWidgetSupervisor
	{
		IGRPanel& panel;
		IGRWidgetCollapserEvents& eventHandler;
		IGRWidgetButton* collapseButton = nullptr;
		IGRWidgetDivision* titleBar = nullptr;
		IGRWidgetDivision* clientArea = nullptr;
		IGRWidgetDivision* leftSpacer = nullptr;
		HString collapserExpandPath = defaultExpandPath;
		HString collapserInlinePath = defaultInlinePath;

		GRCollapser(IGRPanel& owningPanel, IGRWidgetCollapserEvents& _eventHandler) : panel(owningPanel), eventHandler(_eventHandler)
		{
			
		}

		virtual ~GRCollapser()
		{

		}

		void OnTick(float dt) override
		{
			UNUSED(dt);
		}

		bool IsCollapsed() const override
		{
			return collapseButton ? !collapseButton->ButtonFlags().isRaised : false;
		}

		void SetExpandClientAreaImagePath(cstr path) override
		{
			if (path == nullptr || *path == 0)
			{
				collapserExpandPath = defaultExpandPath;
			}
			else
			{
				collapserExpandPath = path;
			}

			collapseButton->SetRaisedImagePath(collapserExpandPath);
		}

		void SetCollapsedToInlineImagePath(cstr path) override
		{
			if (path == nullptr || *path == 0)
			{
				collapserInlinePath = defaultInlinePath;
			}
			else
			{
				collapserInlinePath = path;
			}

			collapseButton->SetPressedImagePath(collapserInlinePath);
		}

		IGRWidgetDivision& ClientArea() override
		{
			return *clientArea;
		}

		IGRWidgetDivision& TitleBar() override
		{
			return *titleBar;
		}

		IGRWidgetDivision& LeftSpacer() override
		{
			return *leftSpacer;
		}

		IGRWidgetButton& CollapseButton() override
		{
			return *collapseButton;
		}

		void PostConstruct()
		{
			panel.SetLayoutDirection(ELayoutDirection::TopToBottom);

			titleBar = &CreateDivision(*this);
			titleBar->Panel().SetExpandToParentHorizontally();
			titleBar->Panel().SetConstantHeight(TITLE_BAR_HEIGHT);
			titleBar->Panel().SetLayoutDirection(ELayoutDirection::LeftToRight);
			titleBar->Panel().Set(GRAnchorPadding{ 0, 1, 0, 1 });

			clientArea = &CreateDivision(*this);
			clientArea->Panel().SetExpandToParentHorizontally();
			clientArea->Panel().SetExpandToParentVertically();

			leftSpacer = &CreateDivision(titleBar->Widget());
			leftSpacer->Panel().SetConstantWidth(0);
			leftSpacer->Panel().SetExpandToParentVertically();

			collapseButton = &CreateButton(titleBar->Widget());
			collapseButton->Widget().Panel().SetExpandToParentVertically();
			collapseButton->Widget().Panel().SetConstantWidth(TITLE_BAR_HEIGHT - 4);
			collapseButton->SetRaisedImagePath(collapserExpandPath);
			collapseButton->SetPressedImagePath(collapserInlinePath);
			collapseButton->SetEventPolicy(EGREventPolicy::NotifyAncestors);
			collapseButton->MakeToggleButton();
			collapseButton->SetStretchImage(true);
		}

		void Free() override
		{
			delete this;
		}

		EGREventRouting OnCursorClick(GRCursorEvent& ce) override
		{
			if (ce.click.LeftButtonUp)
			{
				panel.FocusAndNotifyAncestors();
				return EGREventRouting::Terminate;
			}
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

		IGRPanel& Panel() override
		{
			return panel;
		}

		void Render(IGRRenderContext&) override
		{
		}

		void SyncCollapseStateToButton()
		{
			if (IsCollapsed())
			{
				eventHandler.OnCollapserInlined(*this);
			}
			else
			{
				eventHandler.OnCollapserExpanded(*this);
			}
		}

		EGREventRouting OnChildEvent(GRWidgetEvent&, IGRWidget& sourceWidget)
		{
			if (sourceWidget.Panel().Id() == collapseButton->Widget().Panel().Id())
			{
				SyncCollapseStateToButton();
				return EGREventRouting::Terminate;
			}
			return EGREventRouting::NextHandler;
		}

		EGREventRouting OnKeyEvent(GRKeyEvent& keyEvent) override
		{
			switch (keyEvent.osKeyEvent.VKey)
			{
			case IO::VirtualKeys::VKCode_ENTER:
				if (keyEvent.osKeyEvent.IsUp())
				{
					if (panel.HasFocus())
					{
						auto* child = clientArea->Panel().GetChild(0);
						TrySetDeepFocus(*child);
					}
				}
				return EGREventRouting::Terminate;
			case IO::VirtualKeys::VKCode_SPACEBAR:
				if (keyEvent.osKeyEvent.IsUp())
				{
					if (panel.HasFocus())
					{
						collapseButton->Toggle();
						SyncCollapseStateToButton();
					}
				}				
				return EGREventRouting::Terminate;				
			}
			return collapseButton->Widget().Manager().OnKeyEvent(keyEvent);
		}

		IGRWidget& Widget()
		{
			return *this;
		}

		EGRQueryInterfaceResult QueryInterface(IGRBase** ppOutputArg, cstr interfaceId) override
		{
			GRCollapser* instance = (GRCollapser*)this;
			return Gui::QueryForParticularInterface<IGRWidgetCollapser, GRCollapser>(instance, ppOutputArg, interfaceId);
		}

		cstr GetImplementationTypeName() const override
		{
			return "GRCollapser";
		}
	};

	struct GRCollapserFactory : IGRWidgetFactory
	{
		IGRWidgetCollapserEvents& eventHandler;

		GRCollapserFactory(IGRWidgetCollapserEvents& _eventHandler): eventHandler(_eventHandler)
		{

		}

		IGRWidget& CreateWidget(IGRPanel& panel)
		{
			return *new GRCollapser(panel, eventHandler);
		}
	};
}

namespace Rococo::Gui
{
	ROCOCO_GUI_RETAINED_API cstr IGRWidgetCollapser::InterfaceId()
	{
		return "IGRWidgetCollapser";
	}

	ROCOCO_GUI_RETAINED_API IGRWidgetCollapser& CreateCollapser(IGRWidget& parent, IGRWidgetCollapserEvents& eventHandler)
	{
		auto& gr = parent.Panel().Root().GR();

		GRANON::GRCollapserFactory factory(eventHandler);
		auto& collapser = static_cast<GRANON::GRCollapser&>(gr.AddWidget(parent.Panel(), factory));
		collapser.PostConstruct();
		return collapser;
	}
}