#pragma once

#include <rococo.abstract.editor.h>

#include <rococo.os.win32.h>

namespace Rococo::Windows
{
	struct IParentWindowSupervisor;
}

namespace Rococo::Abedit
{
	ROCOCO_INTERFACE IButtonState
	{
		virtual bool IsChecked() const = 0;
		virtual bool IsGrayed() const = 0;
		virtual void Render(DRAWITEMSTRUCT& d) = 0;
	};

	ROCOCO_INTERFACE IAbeditMainWindowSupervisor : IAbeditMainWindow
	{
		virtual Rococo::Windows::IParentWindowSupervisor& PropertiesPanel() = 0;
		virtual IUIProperties& Properties() = 0;
	};

	namespace Internal
	{
		IAbeditMainWindowSupervisor* CreateMainWindow(HWND hParent, HINSTANCE dllInstance, const EditorSessionConfig& config, IAbstractEditorMainWindowEventHandler& eventHandler);
		IUIBlankSlateSupervisor* CreateBlankSlate();
		IUIPaletteSupervisor* CreatePalette();
		IUIPropertiesSupervisor* CreateProperties(Rococo::Windows::IParentWindowSupervisor& propertiesPanelArea);
	}

	enum { WM_NAVIGATE_BY_TAB = WM_USER + 0x201 };

	HINSTANCE GetAbEditorInstance();
}