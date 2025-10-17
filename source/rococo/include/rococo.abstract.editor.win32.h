// Copyright (c)2025 Mark Anthony Taylor. Email: mark.anthony.taylor@gmail.com. All rights reserved.
#pragma once

#include <rococo.abstract.editor.h>

#include <rococo.os.win32.h>

namespace Rococo::Windows
{
	struct IParentWindowSupervisor;
}

namespace Rococo::Editors
{
	struct IUIPropertiesEditor;
}

namespace Rococo::Abedit
{
	ROCOCO_INTERFACE IAbeditMainWindowSupervisor : IAbeditMainWindow
	{
		virtual Rococo::Windows::IParentWindowSupervisor& PropertiesPanel() = 0;
		virtual Rococo::Editors::IUIPropertiesEditor& Properties() = 0;
		virtual Rococo::Windows::IParentWindowSupervisor& SlateWindow() = 0;
		virtual Rococo::Windows::IWindow& Container() = 0;
		virtual HWND Handle() const = 0;
	};

	namespace Internal
	{
		IAbeditMainWindowSupervisor* CreateMainWindow(HWND hParent, HINSTANCE dllInstance, const EditorSessionConfig& config, IAbstractEditorMainWindowEventHandler& eventHandler, Rococo::Events::IPublisher& publisher);
		IUIPaletteSupervisor* CreatePalette();
	}

	HINSTANCE GetAbEditorInstance();
}