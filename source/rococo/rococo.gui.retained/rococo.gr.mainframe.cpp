#include <rococo.gui.retained.ex.h>
#include <rococo.maths.i32.h>
#include <rococo.ui.h>
#include <rococo.vkeys.h>

#include <vector>
#include <algorithm>

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

		Vec2i lastComputedSpan{ 0,0 };

		void LayoutAfterExpand() override
		{
			if (panel.Span().x > 0 && panel.Span().y > 0)
			{
				if (lastComputedSpan != panel.Span())
				{
					lastComputedSpan = panel.Span();
					SyncToBestZoomLevel();
				}
			}
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

		struct ZoomScenario
		{
			int minScreenWidth;
			int minScreenHeight;

			std::vector<float> zoomLevels;
		};

		std::vector<ZoomScenario> zoomScenarios;

		void AddZoomScenario(int minScreenWidth, int minScreenHeight, const IValueTypeVectorReader<float>& levels) override
		{
			for (auto& target : zoomScenarios)
			{
				if (target.minScreenHeight == minScreenHeight && target.minScreenWidth == minScreenWidth)
				{
					RaiseError(panel, EGRErrorCode::InvalidArg, __FUNCTION__, "Duplicate zoom target span (%d, %d)", minScreenWidth, minScreenHeight);
				}

				if ((target.minScreenWidth > minScreenWidth && target.minScreenHeight < minScreenHeight)
					|| (target.minScreenHeight > minScreenHeight && target.minScreenWidth < minScreenWidth)
					)
				{
					RaiseError(panel, EGRErrorCode::InvalidArg, __FUNCTION__, "Conflicting zoom target span (%d, %d) vs (%d,%d). Width and Height must be both >= or <= other zoom targets", target.minScreenWidth, target.minScreenHeight, minScreenWidth, minScreenHeight);
				}
			}

			size_t nElements = levels.Count();
			if (nElements < 1)
			{
				Throw(0, "No levels specified for ZoomScenario(%d, %d)", minScreenWidth, minScreenHeight);
			}

			ZoomScenario newScenario;
			newScenario.minScreenWidth = minScreenWidth;
			newScenario.minScreenHeight = minScreenHeight;
			newScenario.zoomLevels.resize(nElements);

			for (size_t i = 0; i < nElements; i++)
			{
				newScenario.zoomLevels[i] = levels[i];
			}

			float level0 = newScenario.zoomLevels[0];

			if (level0 < 1.0 || level0 > 100.0f)
			{
				Throw(0, "Level 0 must lie between 1.0 and 100.0");
			}

			float levelIMinus1 = level0;

			for (size_t i = 1; i < nElements; i++)
			{
				float levelI = newScenario.zoomLevels[i];

				if (levelI <= levelIMinus1)
				{
					Throw(0, "Level #d (= %f) must be >= %f, the predecessor", i, levelI, levelIMinus1);
				}

				if (levelI > 100.0)
				{
					Throw(0, "Level #d (= %f) must be <= 100.0", i, levelI);
				}

				levelI = levelIMinus1;
			}

			zoomScenarios.push_back(newScenario);

			// Sort by largest screen ascending
			std::sort(zoomScenarios.begin(), zoomScenarios.end(),
				[](const ZoomScenario& a, const ZoomScenario& b)
				{
					if (a.minScreenWidth < b.minScreenWidth)
					{
						return true;
					}

					if (a.minScreenWidth > b.minScreenWidth)
					{
						return false;
					}

					return a.minScreenHeight < b.minScreenHeight;
				}
			);
		}

		const ZoomScenario* GetBestZoomScenario() const
		{
			Vec2i frameSpan = panel.Span();

			if (zoomScenarios.empty())
			{
				return nullptr;
			}

			auto& z0 = zoomScenarios[0];
			if (frameSpan.x < z0.minScreenWidth || frameSpan.y < z0.minScreenHeight)
			{
				return nullptr;
			}

			for (size_t i = 1; i < zoomScenarios.size(); i++)
			{
				auto& z = zoomScenarios[i];
				if (frameSpan.x < z.minScreenWidth || frameSpan.y < z.minScreenHeight)
				{
					return &zoomScenarios[i-1];
				}
			}

			return &zoomScenarios[zoomScenarios.size() - 1];
		}

		void SyncToBestZoomLevel()
		{
			const ZoomScenario* bestScenario = GetBestZoomScenario();
			if (!bestScenario || bestScenario->zoomLevels.empty())
			{
				panel.Root().Custodian().SetUIZoom(1.0f);
				return;
			}

			float zoomLevel = panel.Root().Custodian().ZoomLevel();

			float maxSupportedZoomLevel = bestScenario->zoomLevels.back();

			if (zoomLevel > maxSupportedZoomLevel)
			{
				panel.Root().Custodian().SetUIZoom(zoomLevel);
			}
		}

		EGREventRouting OnCursorClick(GRCursorEvent& ce) override
		{
			if (ce.click.MouseVWheel && ce.context.isShiftHeld)
			{
				const ZoomScenario* bestScenario = GetBestZoomScenario();

				if (bestScenario)
				{
					auto& zoomLevels = bestScenario->zoomLevels;

					float zoomLevel = panel.Root().Custodian().ZoomLevel();

					if (ce.wheelDelta > 0)
					{
						for (size_t i = 0; i < zoomLevels.size() - 1; i++)
						{
							if (zoomLevel <= zoomLevels[i])
							{
								zoomLevel = zoomLevels[i + 1];
								break;
							}
						}
					}
					else if (ce.wheelDelta < 0)
					{
						for (size_t i = zoomLevels.size() - 1; i > 0; i--)
						{
							if (zoomLevel >= zoomLevels[i])
							{
								zoomLevel = zoomLevels[i - 1];
								break;
							}
						}
					}

					panel.Root().Custodian().SetUIZoom(zoomLevel);
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

		void OnTab(bool ctrlHeld)
		{
			bool nextRatherThanPrevious = !ctrlHeld;
			SetFocusElseRotateFocusToNextSibling(panel, nextRatherThanPrevious);
		}

		void OnReturn()
		{
			MoveFocusIntoChildren(panel);
		}

		EGREventRouting Nav(EGRNavigationDirection direction)
		{
			auto* focusWidget = panel.Root().GR().FindFocusWidget();
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
				OnTab(ke.context.isCtrlHeld);
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
					OnTab(ke.context.isCtrlHeld);
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
					OnTab(ke.context.isCtrlHeld);
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
		GRWidgetRenderState rs(false, false, false);
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
		auto* focusWidget = panel.Root().GR().FindFocusWidget();
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
		auto* focusWidget = panel.Root().GR().FindFocusWidget();
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
		auto* focusWidget = panel.Root().GR().FindFocusWidget();
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

	ROCOCO_GUI_RETAINED_API IGRSystemSubRenderer& GetDefaultFocusRenderer()
	{
		struct FocusRenderer : Gui::IGRSystemSubRenderer
		{
			void Render(IGRPanel& panel, IGRRenderContext& g, const GuiRect& clipRect) override
			{
				g.EnableScissors(clipRect);
				g.DrawRect(panel.AbsRect(), RGBAb(255, 255, 0, 32), panel.RectStyle(), panel.CornerRadius());
				g.DisableScissors();
			}
		}
		
		static s_FocusRenderer;
		return s_FocusRenderer;
	}
}