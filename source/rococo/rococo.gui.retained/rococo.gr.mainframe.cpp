#include <rococo.gui.retained.ex.h>
#include <rococo.maths.i32.h>
#include <rococo.ui.h>
#include <rococo.vkeys.h>

namespace GRANON
{
	using namespace Rococo;
	using namespace Rococo::Gui;

	struct GRMainFrame: IGRWidgetMainFrameSupervisor, IGRWidgetSupervisor, IGRWidgetLayout
	{
		cstr name;
		IGRPanel& panel;
		IGRWidgetDivision* titleBar = nullptr;
		IGRWidgetMenuBar* menuBar = nullptr;
		IGRWidgetToolbar* rhsTools = nullptr;
		IGRWidgetDivision* clientArea = nullptr;

		GRMainFrame(cstr _name, IGRPanel& _panel) : name(_name), panel(_panel)
		{
			_panel.SetMinimalSpan({ 320, 200 });
			_panel.SetLayoutDirection(ELayoutDirection::None);
		}

		void LayoutBeforeFit() override
		{
			MakeTitleBar();
			int titleHeight = titleBar->Widget().Panel().Span().y;
			titleBar->Panel().SetParentOffset({ 0,0 });
			clientArea->Panel().SetParentOffset({ 0, titleHeight });
			clientArea->Panel().SetConstantHeight(panel.Span().y - titleHeight);
			clientArea->Panel().SetConstantWidth(panel.Span().x);
		}

		void LayoutBeforeExpand() override
		{

		}

		void LayoutAfterExpand() override
		{

		}

		void PostConstruct()
		{
			// We construct the client area first, then the title bar to ensure the title bar and menus are rendered after the client area, causing the menus to be on top

			if (!clientArea)
			{
				clientArea = &CreateDivision(*this);
				clientArea->Panel().SetConstantWidth(0);
				clientArea->Panel().SetConstantHeight(0);
				clientArea->Panel().SetDesc("Frame.Client");
			}

			MakeTitleBar();
		}

		void Render(IGRRenderContext& g)
		{
			DrawPanelBackground(panel, g);
		}

		void Free() override
		{
			delete this;
		}

		EGREventRouting OnCursorClick(GRCursorEvent& ce) override
		{
			UNUSED(ce);
			return EGREventRouting::NextHandler;
		}

		EGREventRouting OnChildEvent(GRWidgetEvent&, IGRWidget&) override
		{
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

		EGREventRouting OnEsc()
		{
			return MoveFocusToAncestor(panel);
		}

		void OnReverseTab()
		{
			SetFocusElseRotateFocusToNextSibling(panel, false);
		}

		void OnTab()
		{
			bool nextRatherThanPrevious = !GetCustodian(panel).Keys().IsCtrlPressed();
			SetFocusElseRotateFocusToNextSibling(panel, nextRatherThanPrevious);
		}

		void OnReturn()
		{
			MoveFocusIntoChildren(panel);
		}

		EGREventRouting Nav(EGRNavigationDirection direction)
		{
			auto focusId = panel.Root().GR().GetFocusId();
			auto* focusWidget = panel.Root().GR().FindWidget(focusId);
			if (!focusWidget)
			{
				TrySetDeepFocus(ClientArea().Panel());
				return EGREventRouting::Terminate;
			}

			auto* targetFocus = focusWidget->Panel().Navigate(direction);
			if (targetFocus)
			{
				if (TrySetDeepFocus(*targetFocus) != nullptr)
				{
					return EGREventRouting::Terminate;
				}
			}

			return EGREventRouting::NextHandler;
		}

		EGREventRouting OnKeyEvent(GRKeyEvent& ke) override
		{
			if (ke.osKeyEvent.IsUp())
			{
				switch (ke.osKeyEvent.VKey)
				{
				case Rococo::IO::VirtualKeys::VKCode_ESCAPE:
					return OnEsc();
				case Rococo::IO::VirtualKeys::VKCode_TAB:
					return EGREventRouting::Terminate;
				}
				return EGREventRouting::NextHandler;
			}

			switch (ke.osKeyEvent.VKey)
			{
			case Rococo::IO::VirtualKeys::VKCode_TAB:
				OnTab();
				return EGREventRouting::Terminate;
			case Rococo::IO::VirtualKeys::VKCode_ANTITAB:
				OnReverseTab();
				return EGREventRouting::Terminate;
			case Rococo::IO::VirtualKeys::VKCode_ENTER:
				OnReturn();
				return EGREventRouting::Terminate;
			case Rococo::IO::VirtualKeys::VKCode_LEFT:
				if (Nav(EGRNavigationDirection::Left) == EGREventRouting::NextHandler)
				{
					OnReverseTab();					
				}
				return EGREventRouting::Terminate;
			case Rococo::IO::VirtualKeys::VKCode_RIGHT:
				if (Nav(EGRNavigationDirection::Right) == EGREventRouting::NextHandler)
				{
					OnTab();
				}
				return EGREventRouting::Terminate;
			case Rococo::IO::VirtualKeys::VKCode_UP:
				if (Nav(EGRNavigationDirection::Up) == EGREventRouting::NextHandler)
				{
					OnReverseTab();
				}
				return EGREventRouting::Terminate;
			case Rococo::IO::VirtualKeys::VKCode_DOWN:
				if (Nav(EGRNavigationDirection::Down) == EGREventRouting::NextHandler)
				{
					OnTab();
				}
				return EGREventRouting::Terminate;
			}
			return EGREventRouting::NextHandler;
		}

		IGRPanel& Panel() override
		{
			return panel;
		}

		IGRWidgetDivision& ClientArea() override
		{
			return *clientArea;
		}

		enum { TOOLBAR_PIXEL_HEIGHT_DEFAULT = 30};

		void MakeTitleBar()
		{
			if (!titleBar)
			{
				titleBar = &CreateDivision(*this);
				titleBar->Panel().SetExpandToParentHorizontally();
				titleBar->Panel().SetConstantHeight(TOOLBAR_PIXEL_HEIGHT_DEFAULT);
				titleBar->Panel().SetLayoutDirection(ELayoutDirection::LeftToRight);
				titleBar->Panel().SetDesc("Frame.TitleBar");
			}
		}

		IGRWidgetMenuBar& MenuBar() override
		{
			MakeTitleBar();

			if (!menuBar)
			{
				menuBar = &CreateMenuBar(titleBar->Widget());
				menuBar->Widget().Panel().SetExpandToParentHorizontally();
				menuBar->Widget().Panel().SetExpandToParentVertically();
				menuBar->Widget().Panel().SetDesc("Frame.TitleBar.MenuBar");
			}

			return *menuBar;
		}

		IGRWidgetToolbar& TopRightHandSideTools() override
		{
			MakeTitleBar();

			if (!rhsTools)
			{
				rhsTools = &CreateToolbar(titleBar->Widget());
				rhsTools->Widget().Panel().SetDesc("Frame.TitleBar.RHS");
			}

			return *rhsTools;
		}

		IGRWidget& Widget() override
		{
			return *this;
		}

		IGRWidgetSupervisor& WidgetSupervisor() override
		{
			return *this;
		}

		EGRQueryInterfaceResult QueryInterface(IGRBase** ppOutputArg, cstr interfaceId) override
		{
			auto result = Gui::QueryForParticularInterface<IGRWidgetLayout>(this, ppOutputArg, interfaceId);
			if (result == EGRQueryInterfaceResult::SUCCESS)
			{
				return result;
			}

			return Gui::QueryForParticularInterface<IGRWidgetMainFrame>(this, ppOutputArg, interfaceId);
		}

		cstr GetImplementationTypeName() const override
		{
			return "GRMainFrame";
		}
	};
}

namespace Rococo::Gui
{
	ROCOCO_GUI_RETAINED_API cstr IGRWidgetMainFrame::InterfaceId()
	{
		return "IGRWidgetMainFrame";
	}

	IGRWidgetMainFrameSupervisor* CreateGRMainFrame(cstr name, IGRPanel& panel)
	{
		auto* frame = new GRANON::GRMainFrame(name, panel);
		frame->PostConstruct();
		return frame;
	}

	ROCOCO_GUI_RETAINED_API void DrawPanelBackground(IGRPanel& panel, IGRRenderContext& g)
	{
		GRRenderState rs(false, false, false);
		RGBAb colour = panel.GetColour(EGRSchemeColourSurface::BACKGROUND, rs);
		g.DrawRect(panel.AbsRect(), colour);
	}

	ROCOCO_GUI_RETAINED_API void RotateFocusToNextSibling(IGRWidget& focusWidget, bool nextRatherThanPrevious)
	{
		auto* parent = focusWidget.Panel().Parent();
		if (!parent)
		{
			return;
		}

		int nChildren = parent->EnumerateChildren(nullptr);

		for (int i = 0; i < nChildren; i++)
		{
			auto* child = parent->GetChild(i);
			if (child->Widget() == focusWidget)
			{
				if (nextRatherThanPrevious)
				{
					for (int j = i + 1; j < nChildren; j++)
					{
						auto* sibling = parent->GetChild(j);
						if (TrySetDeepFocus(*sibling))
						{
							return;
						}
					}

					// Nothing following the child was focusable, so try to roll back to the beginning

					if (i > 0)
					{
						for (int k = 0; k < i; k++)
						{
							auto* sibling = parent->GetChild(k);
							if (TrySetDeepFocus(*sibling))
							{
								return;
							}
						}
					}
				}
				else
				{
					for (int j = i - 1; j >= 0; j--)
					{
						auto* sibling = parent->GetChild(j);
						if (TrySetDeepFocus(*sibling))
						{
							return;
						}
					}

					for (int k = nChildren - 1; k > i; k--)
					{
						auto* sibling = parent->GetChild(k);
						if (TrySetDeepFocus(*sibling))
						{
							return;
						}
					}
				}

				// No sibling could take focus, so roll back to container;
				RotateFocusToNextSibling(parent->Widget(), nextRatherThanPrevious);
				return;
			}
		}
	}

	static void SetNewFocus(IGRPanel& panel, bool nextRatherThanPrevious)
	{
		DescAndIndex target = nextRatherThanPrevious ? panel.GetNextNavigationTarget(nullptr) : panel.GetPreviousNavigationTarget(nullptr);
		if (target.desc)
		{
			auto* targetPanel = panel.FindDescendantByDesc(target.desc);
			if (targetPanel)
			{
				TrySetDeepFocus(*targetPanel);
				return;
			}

			RaiseError(panel, EGRErrorCode::InvalidArg, __FUNCTION__ "Unknown navigation target: %s. Source (%s)", target.desc, panel.Desc());
		}

		TrySetDeepFocus(panel);
	}

	ROCOCO_GUI_RETAINED_API void SetFocusElseRotateFocusToNextSibling(IGRPanel& panel, bool nextRatherThanPrevious)
	{
		int64 focusId = panel.Root().GR().GetFocusId();
		if (focusId == -1)
		{
			SetNewFocus(panel, nextRatherThanPrevious);
			return;
		}

		auto* focusWidget = panel.Root().GR().FindWidget(focusId);
		if (focusWidget == nullptr)
		{
			SetNewFocus(panel, nextRatherThanPrevious);
			return;
		}

		cstr desc = focusWidget->Panel().Desc();
		DescAndIndex nextTarget = nextRatherThanPrevious ? panel.GetNextNavigationTarget(desc) : panel.GetPreviousNavigationTarget(desc);
		if (nextTarget.index >= 0)
		{
			auto* nextPanel = panel.FindDescendantByDesc(nextTarget.desc);
			if (nextPanel)
			{
				TrySetDeepFocus(*nextPanel);
				return;
			}

			RaiseError(panel, EGRErrorCode::InvalidArg, __FUNCTION__ "Unknown navigation target: %s. Source (%s)", nextTarget.desc, panel.Desc());
		}

		RotateFocusToNextSibling(*focusWidget, nextRatherThanPrevious);
	}

	ROCOCO_GUI_RETAINED_API void MoveFocusIntoChildren(IGRPanel& panel)
	{
		int64 focusId = panel.Root().GR().GetFocusId();
		if (focusId == -1)
		{
			return;
		}

		auto* focusWidget = panel.Root().GR().FindWidget(focusId);
		if (!focusWidget)
		{
			panel.Root().GR().SetFocus(-1);
			return;
		}

		IGRPanel* newChildFocus = nullptr;

		int nChildren = focusWidget->Panel().EnumerateChildren(nullptr);
		for (int i = 0; i < nChildren; i++)
		{
			auto* child = focusWidget->Panel().GetChild(i);
			newChildFocus = TrySetDeepFocus(*child);
			if (newChildFocus)
			{
				return;
			}
		}

		GetCustodian(panel).AlertNoActionForKey();
	}

	ROCOCO_GUI_RETAINED_API EGREventRouting MoveFocusToAncestor(IGRPanel& panel)
	{
		int64 focusId = panel.Root().GR().GetFocusId();
		if (focusId == -1)
		{
			return EGREventRouting::NextHandler;
		}

		auto* focusWidget = panel.Root().GR().FindWidget(focusId);
		if (!focusWidget)
		{
			panel.Root().GR().SetFocus(-1);
			return EGREventRouting::NextHandler;
		}

		for (auto* ancestor = focusWidget->Panel().Parent(); ancestor != nullptr; ancestor = ancestor->Parent())
		{
			if (ancestor->HasFlag(EGRPanelFlags::AcceptsFocus))
			{
				ancestor->Focus();
				return EGREventRouting::Terminate;
			}
		}

		panel.Root().GR().SetFocus(-1);
		return EGREventRouting::Terminate;
	}
}