#include <rococo.os.win32.h>
#include <rococo.window.h>
#include <rococo.editors.h>
#include <rococo.abstract.editor.h>
#include <rococo.strings.h>
#include <commdlg.h>
#pragma comment(lib, "cfgs.editor.marshaller.lib")
#pragma comment(lib, "rococo.windows.lib")
#include <stdlib.h>

using namespace Rococo::Abedit;

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

	bool TryGetUserSelectedCFGSPath(OUT U8FilePath& path, Abedit::IAbstractEditorSupervisor& editor)
	{
		auto& super = static_cast<Abedit::IWin32AbstractEditorSupervisor&>(editor);
		HWND hRoot = GetAncestor(super.Slate(), GA_ROOT);

		OPENFILENAMEA spec = { 0 };
		spec.lStructSize = sizeof(spec);
		spec.hwndOwner = hRoot;
		spec.lpstrFilter = "control-flow graph SXML file\0*.cfgs.sxml\0\0";
		spec.nFilterIndex = 1;

		spec.lpstrFile = path.buf;
		*path.buf = 0;
		spec.nMaxFile = sizeof path;

		char title[256];
		Strings::SecureFormat(title, sizeof(title), "Select a flow graph file");
		spec.lpstrTitle = title;

		spec.Flags = 0;

		char currentDirectory[_MAX_PATH];
		GetCurrentDirectoryA(_MAX_PATH, currentDirectory);

		bool isOpen = false;

		if (GetOpenFileNameA(&spec))
		{
			SetWindowTextA(hRoot, spec.lpstrFile);
			isOpen = true;
		}

		SetCurrentDirectoryA(currentDirectory);

		return isOpen;
	}
}

