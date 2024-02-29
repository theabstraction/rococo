#include <rococo.os.win32.h>
#include <rococo.window.h>
#include <rococo.editors.h>
#include <rococo.abstract.editor.h>
#include <rococo.strings.h>

namespace Rococo::CFGS
{
	Editors::IUI2DGridSlateSupervisor* Create2DGridControl(Abedit::IAbstractEditorSupervisor& editor, Rococo::Editors::IUI2DGridEvents& eventHandler)
	{
		if (!Strings::Eq(editor.Implementation(), IMPLEMENTATION_TYPE_WIN32_HWND))
		{
			Throw(0, "%s: implementation needs to be %s", __FUNCTION__, IMPLEMENTATION_TYPE_WIN32_HWND);
		}

		auto& super = static_cast<Abedit::IWin32AbstractEditorSupervisor&>(editor);
		return Rococo::Windows::Create2DGrid(super.Slate(), WS_VISIBLE | WS_CHILD, eventHandler, true);
	}
}

#pragma comment(lib, "rococo.windows.lib")