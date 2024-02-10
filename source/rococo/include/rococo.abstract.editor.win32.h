#pragma once

#include <rococo.abstract.editor.h>

#include <rococo.os.win32.h>

namespace Rococo::Windows
{
	struct IParentWindowSupervisor;
}

namespace Rococo::Abedit
{
	ROCOCO_INTERFACE IAbeditMainWindowSupervisor : IAbeditMainWindow
	{
		virtual Rococo::Windows::IParentWindowSupervisor& PropertiesPanel() = 0;
	};

	namespace Internal
	{
		IAbeditMainWindowSupervisor* CreateMainWindow(HWND hParent, HINSTANCE dllInstance, const EditorSessionConfig& config, IAbstractEditorMainWindowEventHandler& eventHandler, IUIPropertyEvents& propertyEvents);
		IUIBlankSlateSupervisor* CreateBlankSlate();
		IUIPaletteSupervisor* CreatePalette();
		IUIPropertiesSupervisor* CreateProperties(Rococo::Windows::IParentWindowSupervisor& propertiesPanelArea);
	}
}