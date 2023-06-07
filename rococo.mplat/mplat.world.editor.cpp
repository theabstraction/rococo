#include <rococo.gui.retained.ex.h>
#include "mplat.editor.h"
#include <rococo.reflector.h>
#include <rococo.strings.h>
#include <vector>
#include <rococo.mplat.h>
#include <rococo.task.queue.h>

using namespace Rococo;
using namespace Rococo::Gui;
using namespace Rococo::MPEditor;
using namespace Rococo::Reflection;
using namespace Rococo::Strings;

namespace ANON
{
	void BuildMenus(IGRWidgetMainFrame& frame)
	{
		auto& menu = frame.MenuBar();
		menu.Widget().Panel().
			Set(ESchemeColourSurface::MENU_BUTTON_EDGE_BOTTOM_RIGHT, RGBAb(0, 0, 0, 0)).
			Set(ESchemeColourSurface::MENU_BUTTON_EDGE_TOP_LEFT, RGBAb(0, 0, 0, 0));

		auto fileMenu = menu.AddSubMenu(GRMenuItemId::Root(), GRMenuSubMenu("File"));
		auto editMenu = menu.AddSubMenu(GRMenuItemId::Root(), GRMenuSubMenu("Edit"));
		auto viewMenu = menu.AddSubMenu(GRMenuItemId::Root(), GRMenuSubMenu("View"));
		auto projectMenu = menu.AddSubMenu(GRMenuItemId::Root(), GRMenuSubMenu("Project"));
		auto windowMenu = menu.AddSubMenu(GRMenuItemId::Root(), GRMenuSubMenu("Window"));
		auto helpMenu = menu.AddSubMenu(GRMenuItemId::Root(), GRMenuSubMenu("Help"));

		menu.AddButton(fileMenu, { "New", { 0, nullptr } });
		menu.AddButton(fileMenu, { "Open...", { 0, nullptr } });
		menu.AddButton(fileMenu, { "Save", { 0, nullptr } });
		menu.AddButton(fileMenu, { "Save As...", { 0, nullptr } });
		menu.AddButton(fileMenu, { "Exit", { 0, nullptr } });

		menu.AddButton(editMenu, { "Find...", { 0, nullptr } });
		menu.AddButton(editMenu, { "Replace...", { 0, nullptr } });
		menu.AddButton(editMenu, { "Copy", { 0, nullptr } });
		menu.AddButton(editMenu, { "Cut", { 0, nullptr } });
		menu.AddButton(editMenu, { "Paste", { 0, nullptr } });

		menu.AddButton(viewMenu, { "Solution", { 0, nullptr } });
		menu.AddButton(viewMenu, { "Classes", { 0, nullptr } });
		menu.AddButton(viewMenu, { "Repo", { 0, nullptr } });
		menu.AddButton(viewMenu, { "Debugger", { 0, nullptr } });
		menu.AddButton(viewMenu, { "Output", { 0, nullptr } });

		menu.AddButton(projectMenu, { "Build", { 0, nullptr } });
		menu.AddButton(projectMenu, { "Rebuild", { 0, nullptr } });
		menu.AddButton(projectMenu, { "Debug", { 0, nullptr } });
		menu.AddButton(projectMenu, { "Cancel", { 0, nullptr } });

		menu.AddButton(windowMenu, { "Split", { 0, nullptr } });
		menu.AddButton(windowMenu, { "Cascade", { 0, nullptr } });
		menu.AddButton(windowMenu, { "Merge", { 0, nullptr } });

		auto toggles = menu.AddSubMenu(windowMenu, GRMenuSubMenu("Toggles"));
		menu.AddButton(toggles, { "Toolkit", { 0, nullptr } });
		menu.AddButton(toggles, { "Properties", { 0, nullptr } });
		menu.AddButton(toggles, { "Log", { 0, nullptr } });
		menu.AddButton(windowMenu, { "Close All", { 0, nullptr } });

		menu.AddButton(helpMenu, { "About...", { 0, nullptr } });
		menu.AddButton(helpMenu, { "Check for updates", { 0, nullptr } });
		menu.AddButton(helpMenu, { "Version", { 0, nullptr } });
		menu.AddButton(helpMenu, { "Purchase License", { 0, nullptr } });

		auto& titleBar = *frame.MenuBar().Widget().Panel().Parent();
		titleBar.Set(ESchemeColourSurface::CONTAINER_BACKGROUND, RGBAb(0, 0, 0, 255)).Set(ESchemeColourSurface::CONTAINER_BACKGROUND_HOVERED, RGBAb(8, 8, 8, 255));
		titleBar.Set(ESchemeColourSurface::MENU_BUTTON_RAISED, RGBAb(0, 0, 0, 255)).Set(ESchemeColourSurface::MENU_BUTTON_PRESSED, RGBAb(8, 8, 8, 255));
		titleBar.Set(ESchemeColourSurface::BUTTON_RAISED, RGBAb(0, 0, 0, 255)).Set(ESchemeColourSurface::BUTTON_PRESSED, RGBAb(24, 24, 24, 255));
		titleBar.Set(ESchemeColourSurface::BUTTON_RAISED_AND_HOVERED, RGBAb(32, 32, 32, 255)).Set(ESchemeColourSurface::BUTTON_PRESSED_AND_HOVERED, RGBAb(48, 48, 48, 255));
		titleBar.Set(ESchemeColourSurface::MENU_BUTTON_RAISED_AND_HOVERED, RGBAb(16, 16, 16, 255)).Set(ESchemeColourSurface::MENU_BUTTON_PRESSED_AND_HOVERED, RGBAb(32, 32, 32, 255));
		titleBar.Set(ESchemeColourSurface::IMAGE_FOG_HOVERED, RGBAb(0, 0, 0, 64)).Set(ESchemeColourSurface::IMAGE_FOG, RGBAb(0, 0, 0, 128));
	}

	enum { TOOLBAR_EVENT_MINIMIZE = 40001, TOOLBAR_EVENT_RESTORE, TOOLBAR_EVENT_EXIT };

	void BuildUpperRightToolbar(IGRWidgetMainFrame& frame)
	{
		auto& tools = frame.TopRightHandSideTools();
		tools.SetChildAlignment(GRAlignment::Right);
		
		auto& minimizer = CreateButton(tools.Widget()).SetTitle("Min").SetImagePath("!textures/toolbars/3rd-party/www.aha-soft.com/Down.tiff").SetClickCriterion(GRClickCriterion::OnDownThenUp).SetEventPolicy(GREventPolicy::PublicEvent);
		auto& restorer = CreateButton(tools.Widget()).SetTitle("Max").SetImagePath("!textures/toolbars/3rd-party/www.aha-soft.com/Expand.tiff").SetClickCriterion(GRClickCriterion::OnDownThenUp).SetEventPolicy(GREventPolicy::PublicEvent);
		auto& closer = CreateButton(tools.Widget()).SetTitle("Close").SetImagePath("!textures/toolbars/3rd-party/www.aha-soft.com/Close.tiff").SetClickCriterion(GRClickCriterion::OnDownThenUp).SetEventPolicy(GREventPolicy::PublicEvent);

		minimizer.SetMetaData({ TOOLBAR_EVENT_MINIMIZE, "OnMinimize" });
		restorer.SetMetaData({ TOOLBAR_EVENT_RESTORE, "OnMinimize" });
		closer.SetMetaData({ TOOLBAR_EVENT_EXIT, "OnExit" });

		tools.ResizeToFitChildren();
	}

	struct MPlatEditor : IMPEditorSupervisor, IGREventHandler
	{
		IGuiRetained& gr;
		Platform* platform = nullptr;
		bool isVisible = false;

		MPlatEditor(IGuiRetained& _gr): gr(_gr)
		{
			static_cast<IGuiRetainedSupervisor&>(gr).SetEventHandler(this);
		}

		void SetPlatform(Platform* platform)
		{
			this->platform = platform;
		}

		void OnButtonClickTaskResult(int64 code)
		{
			switch (code)
			{
			case TOOLBAR_EVENT_MINIMIZE:
				platform->graphics.renderer.SwitchToWindowMode();
				Rococo::Windows::MinimizeApp(platform->mainWindow);
				break;
			case TOOLBAR_EVENT_RESTORE:
				if (platform->graphics.renderer.IsFullscreen())
				{
					platform->graphics.renderer.SwitchToWindowMode();
				}
				else
				{
					platform->graphics.renderer.SwitchToFullscreen();
				}
				break;
			case TOOLBAR_EVENT_EXIT:
				if (platform->graphics.renderer.IsFullscreen())
				{
					platform->graphics.renderer.SwitchToWindowMode();
				}

				Rococo::Windows::SendCloseEvent(platform->mainWindow);
				break;
			}
		}

		void OnButtonClick(WidgetEvent& buttonEvent)
		{
			int64 id = buttonEvent.iMetaData;
			switch (id)
			{
			case TOOLBAR_EVENT_MINIMIZE:
				platform->graphics.renderer.SwitchToWindowMode();
				Rococo::Windows::MinimizeApp(platform->mainWindow);
				break;
			case TOOLBAR_EVENT_RESTORE:
				if (platform->graphics.renderer.IsFullscreen())
				{
					platform->graphics.renderer.SwitchToWindowMode();
				}
				else
				{
					platform->graphics.renderer.SwitchToFullscreen();
				}
				break;
			case TOOLBAR_EVENT_EXIT:
				if (platform->graphics.renderer.IsFullscreen())
				{
					platform->graphics.renderer.SwitchToWindowMode();
				}

				Rococo::Windows::SendCloseEvent(platform->mainWindow);
				break;
			}
		}

		EventRouting OnGREvent(WidgetEvent& ev) override
		{
			switch (ev.eventType)
			{
			case WidgetEventType::BUTTON_CLICK:
				OnButtonClick(ev);
				break;
			}
			return EventRouting::Terminate;
		}

		void Free() override
		{
			delete this;
		}

		bool IsVisible() const override
		{
			return isVisible;
		}

		void SetVisibility(bool isVisible) override
		{
			this->isVisible = isVisible;

			if (isVisible)
			{
				InstantiateUI();
				gr.SetVisible(true);
			}
			else
			{
				gr.SetVisible(false);
			}
		}

		void ClearFrame(IGRWidgetMainFrame& frame)
		{
			struct : IEventCallback<IGRPanel>
			{
				void OnEvent(IGRPanel& panel) override
				{
					panel.MarkForDelete();
				}
			} cb;
			frame.ClientArea().Panel().EnumerateChildren(&cb);
			frame.ClientArea().Panel().Root().GR().GarbageCollect();
		}

		IdWidget ID_EDITOR_FRAME = { "MPlat-MainFrame" };

		void InstantiateUI()
		{
			auto& frame = gr.BindFrame(ID_EDITOR_FRAME);
			auto& scheme = gr.Root().Scheme();
			SetSchemeColours_ThemeGrey(scheme);
			BuildMenus(frame);
			BuildUpperRightToolbar(frame);

			auto& framePanel = frame.Widget().Panel();

			framePanel.Set(ESchemeColourSurface::TEXT, RGBAb(224, 224, 224, 255)).Set(ESchemeColourSurface::TEXT_HOVERED, RGBAb(255, 255, 255, 255));
			framePanel.Set(ESchemeColourSurface::EDIT_TEXT, RGBAb(224, 224, 224, 255)).Set(ESchemeColourSurface::EDIT_TEXT_HOVERED, RGBAb(255, 255, 255, 255));
			framePanel.Set(ESchemeColourSurface::FOCUSED_EDITOR, RGBAb(0, 0, 0, 255));
			framePanel.Set(ESchemeColourSurface::FOCUSED_EDITOR_HOVERED, RGBAb(16, 16, 16, 255));
			framePanel.Set(ESchemeColourSurface::SCROLLER_BUTTON_BACKGROUND, RGBAb(64, 64, 64, 255)).Set(ESchemeColourSurface::SCROLLER_BUTTON_BACKGROUND_HOVERED, RGBAb(96, 96, 96, 255));
			framePanel.Set(ESchemeColourSurface::SCROLLER_BUTTON_TOP_LEFT, RGBAb(192, 192, 192, 255)).Set(ESchemeColourSurface::SCROLLER_BUTTON_TOP_LEFT_HOVERED, RGBAb(255, 255, 255, 255));
			framePanel.Set(ESchemeColourSurface::SCROLLER_BUTTON_BOTTOM_RIGHT, RGBAb(128, 128, 128, 255)).Set(ESchemeColourSurface::SCROLLER_BUTTON_BOTTOM_RIGHT_HOVERED, RGBAb(192, 192, 192, 255));
			framePanel.Set(ESchemeColourSurface::SCROLLER_BAR_BACKGROUND, RGBAb(64, 64, 64, 255)).Set(ESchemeColourSurface::SCROLLER_BAR_BACKGROUND_HOVERED, RGBAb(72, 72, 72, 255));
			framePanel.Set(ESchemeColourSurface::SCROLLER_BAR_TOP_LEFT, RGBAb(128, 128, 128, 255)).Set(ESchemeColourSurface::SCROLLER_BAR_TOP_LEFT_HOVERED, RGBAb(136, 136, 136, 255));
			framePanel.Set(ESchemeColourSurface::SCROLLER_BAR_BOTTOM_RIGHT, RGBAb(96, 96, 96, 255)).Set(ESchemeColourSurface::SCROLLER_BAR_BOTTOM_RIGHT_HOVERED, RGBAb(160, 160, 160, 255));
			framePanel.Set(ESchemeColourSurface::SCROLLER_SLIDER_BACKGROUND, RGBAb(64, 64, 64, 255)).Set(ESchemeColourSurface::SCROLLER_SLIDER_BACKGROUND_HOVERED, RGBAb(192, 192, 192, 255));
			framePanel.Set(ESchemeColourSurface::SCROLLER_SLIDER_TOP_LEFT, RGBAb(128, 128, 128, 255)).Set(ESchemeColourSurface::SCROLLER_SLIDER_TOP_LEFT_HOVERED, RGBAb(255, 255, 255, 255));
			framePanel.Set(ESchemeColourSurface::SCROLLER_SLIDER_BOTTOM_RIGHT, RGBAb(96, 96, 96, 255)).Set(ESchemeColourSurface::SCROLLER_SLIDER_BOTTOM_RIGHT_HOVERED, RGBAb(224, 224, 224, 255));
			framePanel.Set(ESchemeColourSurface::SCROLLER_TRIANGLE_NORMAL, RGBAb(128, 128, 128, 255)).Set(ESchemeColourSurface::SCROLLER_TRIANGLE_HOVERED, RGBAb(192, 192, 192, 255));

			auto& custodian = gr.Root().Custodian();
		}

		void Preview(IGuiRetained& gr, IReflectionTarget& target) override
		{
			auto* frame = gr.FindFrame(ID_EDITOR_FRAME);
			if (!frame) Throw(0, "%s: Unexpected missing frame. gr.FindFrame(ID_EDITOR_FRAME) returned null", __FUNCTION__);

			ClearFrame(*frame);

			auto& frameSplitter = CreateLeftToRightSplitter(frame->ClientArea(), 240, false).SetDraggerMinMax(240, 8192);
			frameSplitter.Widget().Panel().Add(GRAnchors::ExpandAll());

			IGRWidgetPropertyEditorTree& editorTree = CreatePropertyEditorTree(frameSplitter.First(), target);
			editorTree.Widget().Panel().Add(GRAnchors::ExpandAll());
		}
	};
}

namespace Rococo::MPEditor
{
	IMPEditorSupervisor* CreateMPlatEditor(Rococo::Gui::IGuiRetained& gr)
	{
		return new ANON::MPlatEditor(gr);
	}
}
