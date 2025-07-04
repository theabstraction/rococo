#include <rococo.gui.retained.ex.h>
#include "mplat.editor.h"
#include <rococo.reflector.h>
#include <rococo.strings.h>
#include <vector>
#include <rococo.mplat.h>
#include <rococo.task.queue.h>
#include <rococo.reflector.h>
#include <unordered_set>

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
		menu.Panel().
			Set(EGRSchemeColourSurface::MENU_BUTTON_EDGE_BOTTOM_RIGHT, RGBAb(0, 0, 0, 0), EGRColourSpec::ForAllRenderStates).
			Set(EGRSchemeColourSurface::MENU_BUTTON_EDGE_TOP_LEFT, RGBAb(0, 0, 0, 0), EGRColourSpec::ForAllRenderStates);

		auto fileMenu = menu.AddSubMenu(GRMenuItemId::Root(), GRMenuSubMenu("File"));

		/*
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

		*/

		auto& titleBar = *frame.MenuBar().Panel().Parent();
		titleBar.Set(EGRSchemeColourSurface::CONTAINER_BACKGROUND, RGBAb(0, 0, 0, 255), EGRColourSpec::ForAllRenderStates);
		titleBar.Set(EGRSchemeColourSurface::MENU_BUTTON, RGBAb(0, 0, 0, 255), EGRColourSpec::ForAllRenderStates);
		titleBar.Set(EGRSchemeColourSurface::BUTTON, RGBAb(0, 0, 0, 255), EGRColourSpec::ForAllRenderStates);
	}

	void BuildUpperRightToolbar(IGRWidgetMainFrame& frame)
	{
		auto& tools = frame.TopRightHandSideTools();
		tools.SetChildAlignment(EGRAlignment::Right);
		
		auto& minimizer = CreateButton(tools.Widget()).SetTitle("Min").SetImagePath("!textures/toolbars/3rd-party/www.aha-soft.com/Down.tiff").SetClickCriterion(EGRClickCriterion::OnDownThenUp).SetEventPolicy(EGREventPolicy::PublicEvent);
		auto& restorer = CreateButton(tools.Widget()).SetTitle("Max").SetImagePath("!textures/toolbars/3rd-party/www.aha-soft.com/Expand.tiff").SetClickCriterion(EGRClickCriterion::OnDownThenUp).SetEventPolicy(EGREventPolicy::PublicEvent);
		auto& closer = CreateButton(tools.Widget()).SetTitle("Close").SetImagePath("!textures/toolbars/3rd-party/www.aha-soft.com/Close.tiff").SetClickCriterion(EGRClickCriterion::OnDownThenUp).SetEventPolicy(EGREventPolicy::PublicEvent);

		minimizer.SetMetaData({  (int64) ToolbarMetaId::MINIMIZE, "OnMinimize" }, true);
		restorer.SetMetaData({ (int64) ToolbarMetaId::RESTORE, "OnRestore" }, true);
		closer.SetMetaData({ (int64) ToolbarMetaId::EXIT, "OnExit" }, true);

		tools.ResizeToFitChildren();
	}

	struct MPlatEditor : IMPEditorSupervisor, IGREventHandler, IReflectionTarget
	{
		IGRSystem& gr;
		Platform* platform = nullptr;
		bool isVisible = false;
		std::unordered_set<IMPEditorEventHandler*> hooks;
		Reflection::Visitation visitation;

		MPlatEditor(IGRSystem& _gr): gr(_gr), visitation(*this)
		{
			static_cast<IGRSystemSupervisor&>(gr).SetEventHandler(this);
		}

		void SetPlatform(Platform* platform)
		{
			this->platform = platform;
		}

		void AddHook(IMPEditorEventHandler* eventHandler) override
		{
			hooks.insert(eventHandler);
		}

		void RemoveHook(IMPEditorEventHandler* eventHandler) override
		{
			hooks.erase(eventHandler);
		}

		void OnButtonClickTaskResult(int64 code)
		{
			switch ((ToolbarMetaId) code)
			{
			case ToolbarMetaId::MINIMIZE:
				platform->graphics.renderer.SwitchToWindowMode();
				Rococo::Windows::MinimizeApp(platform->os.mainWindow);
				break;
			case ToolbarMetaId::RESTORE:
				if (platform->graphics.renderer.IsFullscreen())
				{
					platform->graphics.renderer.SwitchToWindowMode();
				}
				else
				{
					platform->graphics.renderer.SwitchToFullscreen();
				}
				break;
			case ToolbarMetaId::EXIT:
				if (platform->graphics.renderer.IsFullscreen())
				{
					platform->graphics.renderer.SwitchToWindowMode();
				}

				Rococo::Windows::SendCloseEvent(platform->os.mainWindow);
				break;
			}
		}

		void OnButtonClick(GRWidgetEvent& buttonEvent)
		{
			int64 id = buttonEvent.iMetaData;
			switch ((ToolbarMetaId) id)
			{
			case ToolbarMetaId::MINIMIZE:
				platform->graphics.renderer.SwitchToWindowMode();
				Rococo::Windows::MinimizeApp(platform->os.mainWindow);
				return;
			case ToolbarMetaId::RESTORE:
				if (platform->graphics.renderer.IsFullscreen())
				{
					platform->graphics.renderer.SwitchToWindowMode();
				}
				else
				{
					platform->graphics.renderer.SwitchToFullscreen();
				}
				return;
			case ToolbarMetaId::EXIT:
				if (platform->graphics.renderer.IsFullscreen())
				{
					platform->graphics.renderer.SwitchToWindowMode();
				}

				Rococo::Windows::SendCloseEvent(platform->os.mainWindow);
				return;
			}

			for (auto* hook : hooks)
			{
				hook->OnMPEditor_ButtonClick(buttonEvent);
			}
		}

		EGREventRouting OnGREvent(GRWidgetEvent& ev) override
		{
			switch (ev.eventType)
			{
			case EGRWidgetEventType::BUTTON_CLICK:
				OnButtonClick(ev);
				break;
			}
			return EGREventRouting::Terminate;
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

		GRIdWidget ID_EDITOR_FRAME = { "MPlat-MainFrame" };

		bool isInitialized = false;

		void InstantiateUI()
		{
			if (!isInitialized)
			{
				isInitialized = true;
			}
			else
			{
				return;
			}

			auto& frame = gr.BindFrame(ID_EDITOR_FRAME);
			auto& scheme = gr.Root().Scheme();
			SetSchemeColours_ThemeGrey(scheme);
			BuildMenus(frame);
			BuildUpperRightToolbar(frame);

			auto& framePanel = frame.Panel();

			framePanel.Set(EGRSchemeColourSurface::SCROLLER_BUTTON_BACKGROUND, RGBAb(48, 48, 48, 255), EGRColourSpec::ForAllRenderStates);
			framePanel.Set(EGRSchemeColourSurface::SCROLLER_BUTTON_TOP_LEFT, RGBAb(128, 128, 128, 255), EGRColourSpec::ForAllRenderStates);
			framePanel.Set(EGRSchemeColourSurface::SCROLLER_BUTTON_BOTTOM_RIGHT, RGBAb(96, 96, 96, 255), EGRColourSpec::ForAllRenderStates);
			framePanel.Set(EGRSchemeColourSurface::SCROLLER_BAR_BACKGROUND, RGBAb(48, 48, 48, 255), EGRColourSpec::ForAllRenderStates);
			framePanel.Set(EGRSchemeColourSurface::SCROLLER_BAR_TOP_LEFT, RGBAb(120, 120, 120, 255), EGRColourSpec::ForAllRenderStates);
			framePanel.Set(EGRSchemeColourSurface::SCROLLER_BAR_BOTTOM_RIGHT, RGBAb(104, 104, 104, 255), EGRColourSpec::ForAllRenderStates);
			framePanel.Set(EGRSchemeColourSurface::SCROLLER_SLIDER_BACKGROUND, RGBAb(64, 64, 64, 255), EGRColourSpec::ForAllRenderStates);
			framePanel.Set(EGRSchemeColourSurface::SCROLLER_SLIDER_TOP_LEFT, RGBAb(128, 128, 128, 255), EGRColourSpec::ForAllRenderStates);
			framePanel.Set(EGRSchemeColourSurface::SCROLLER_SLIDER_BOTTOM_RIGHT, RGBAb(96, 96, 96, 255), EGRColourSpec::ForAllRenderStates);
			framePanel.Set(EGRSchemeColourSurface::SCROLLER_TRIANGLE_NORMAL, RGBAb(128, 128, 128, 255), EGRColourSpec::ForAllRenderStates);
		}

		void Preview(IGRSystem& gr, IReflectionVisitation* visitation) override
		{
			if (!visitation)
			{
				Throw(0, "visitation was null");
			}

			auto* frame = gr.FindFrame(ID_EDITOR_FRAME);
			if (!frame) Throw(0, "%s: Unexpected missing frame. gr.FindFrame(ID_EDITOR_FRAME) returned null", __ROCOCO_FUNCTION__);

			ClearFrame(*frame);
			
			auto& frameSplitter = CreateLeftToRightSplitter(frame->ClientArea().Widget(), 640, true).SetDraggerMinMax(240, 8192);

			SetPropertyEditorColours_PastelScheme(frameSplitter.Panel());

			frameSplitter.EvOnSplitSizeChanged().Add(
				[this](int32 newSplitterWidth)
				{
					this->splitterWidth = newSplitterWidth;
				}
			);

			struct PropEditorPopulatorHandler : IGRPropertyEditorPopulationEvents
			{
				void OnAddNameValue(IGRWidgetText& /* nameWidget */, IGRWidgetEditBox& /* editorWidget */) override
				{
					
				}
			} popHandler;

			PropertyEditorSpec spec;

			FontSpec boldFont;
			boldFont.Bold = true;
			boldFont.CharHeight = 16;
			boldFont.CharSet = ECharSet::ANSI;
			boldFont.FontName = "Consolas";
			spec.NameplateFontId = gr.Fonts().BindFontId(boldFont);

			FontSpec headingFontSpec;
			headingFontSpec.Bold = true;
			headingFontSpec.CharHeight = 20;
			headingFontSpec.CharSet = ECharSet::ANSI;
			headingFontSpec.FontName = "Consolas";
			spec.HeadingFontId = gr.Fonts().BindFontId(headingFontSpec);

			FontSpec valueFontSpec;
			valueFontSpec.Bold = false;
			valueFontSpec.CharHeight = 16;
			valueFontSpec.CharSet = ECharSet::ANSI;
			valueFontSpec.FontName = "Consolas";
			spec.ValueFontId = gr.Fonts().BindFontId(valueFontSpec);

			spec.LeftAlignNameplates = true;

			IGRWidgetPropertyEditorTree& editorTree = CreatePropertyEditorTree(frameSplitter.First().Widget(), popHandler, spec);
			editorTree.SetRowHeight(gr.Fonts().GetFontHeight(spec.NameplateFontId) + 4);
			editorTree.View(visitation);
		}

		IReflectionVisitation* Visitation() override
		{
			return &visitation;
		}

		Reflection::IReflectionTarget& ReflectionTarget() override
		{
			return *this;
		}

		int splitterWidth = 120;

		void Visit(IReflectionVisitor& v) override
		{
			ROCOCO_REFLECT(v, splitterWidth);
		}
	};
}

namespace Rococo::MPEditor
{
	IMPEditorSupervisor* CreateMPlatEditor(Rococo::Gui::IGRSystem& gr)
	{
		return new ANON::MPlatEditor(gr);
	}
}
