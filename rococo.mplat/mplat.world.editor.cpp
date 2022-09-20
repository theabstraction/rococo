#include <rococo.gui.retained.h>
#include "mplat.editor.h"
#include <rococo.reflector.h>

using namespace Rococo;
using namespace Rococo::Gui;
using namespace Rococo::MPEditor;
using namespace Rococo::Reflection;

namespace ANON
{
	void BuildMenus(IGRMainFrame& frame)
	{
		auto& menu = frame.GetMenuBar();
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
	}

	void BuildUpperRightToolbar(IGRMainFrame& frame)
	{
		auto& tools = frame.GetTopRightHandSideTools();
		tools.SetChildAlignment(GRAlignment::Right);
		
		CreateButton(tools).SetTitle("Min").SetImagePath("!textures/toolbars/3rd-party/www.aha-soft.com/Down.tiff");
		CreateButton(tools).SetTitle("Max").SetImagePath("!textures/toolbars/3rd-party/www.aha-soft.com/Expand.tiff");
		CreateButton(tools).SetTitle("Close").SetImagePath("!textures/toolbars/3rd-party/www.aha-soft.com/Close.tiff");

		tools.ResizeToFitChildren();
	}

	struct MPlatEditor : IMPEditorSupervisor
	{
		IGuiRetained& gr;
		bool isVisible = false;

		MPlatEditor(IGuiRetained& _gr): gr(_gr)
		{

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

		void InstantiateUI()
		{
			auto& frame = gr.BindFrame(IdWidget{ "MPlat-MainFrame" });
			auto& scheme = gr.Root().Scheme();
			SetSchemeColours_ThemeGrey(scheme);
			BuildMenus(frame);
			BuildUpperRightToolbar(frame);
		}

		void Reflect(IReflectionTarget& target) override
		{

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
