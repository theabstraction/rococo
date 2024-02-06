#pragma once

#include <rococo.abstract.editor.h>

#include <rococo.os.win32.h>

namespace Rococo::Abedit
{
	namespace Internal
	{
		IAbeditMainWindow* CreateMainWindow(HWND hParent, HINSTANCE dllInstance, const EditorSessionConfig& config, IAbstractEditorMainWindowEventHandler& eventHandler);
	}
}