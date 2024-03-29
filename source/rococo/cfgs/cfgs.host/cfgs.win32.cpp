#include <rococo.os.win32.h>
#include <rococo.window.h>
#include <rococo.editors.h>
#include <rococo.abstract.editor.h>
#include <rococo.strings.h>
#include <commdlg.h>
#pragma comment(lib, "cfgs.editor.marshaller.lib")
#pragma comment(lib, "rococo.windows.lib")
#include <stdlib.h>
#include <rococo.cfgs.h>

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

	typedef ICFGSIntegratedDevelopmentEnvironmentSupervisor* (*FN_Create_CFGS_Win32_IDE)(HWND hHostWindow, ICFGSDatabase& db, Rococo::Abedit::IAbstractEditor& editor);

	ICFGSIntegratedDevelopmentEnvironmentSupervisor* Create_CFGS_IDE(IAbstractEditorSupervisor& editor, ICFGSDatabase& db)
	{
		auto& super = static_cast<Abedit::IWin32AbstractEditorSupervisor&>(editor);
		HWND hRoot = GetAncestor(super.Slate(), GA_ROOT);

		cstr dllname = "cfgs.sexy.ide.dll";

		HMODULE hPopupModule = LoadLibraryA(dllname);
		if (!hPopupModule)
		{
			Throw(GetLastError(), "%s: Could not load library: %s", __FUNCTION__, dllname);
		}

		cstr procName = "Create_CFGS_Win32_IDE";

		FARPROC factoryProc = GetProcAddress(hPopupModule, procName);
		if (!factoryProc)
		{
			Throw(GetLastError(), "%s: Could not load find '%s' in %s", __FUNCTION__, procName, dllname);
		}

		FN_Create_CFGS_Win32_IDE createIDE = (FN_Create_CFGS_Win32_IDE)factoryProc;

		return createIDE(hRoot, db, editor);
	}

	bool TryGetUserSelectedCFGSPath(OUT WideFilePath& path, Abedit::IAbstractEditorSupervisor& editor)
	{
		auto& super = static_cast<Abedit::IWin32AbstractEditorSupervisor&>(editor);
		HWND hRoot = GetAncestor(super.Slate(), GA_ROOT);

		OPENFILENAMEW spec = { 0 };
		spec.lStructSize = sizeof(spec);
		spec.hwndOwner = hRoot;
		spec.lpstrFilter = L"control-flow graph SXML file\0*.cfgs.sxml\0\0";
		spec.nFilterIndex = 1;

		spec.lpstrFile = path.buf;
		*path.buf = 0;
		spec.nMaxFile = sizeof path / sizeof(wchar_t);

		wchar_t title[256];
		Strings::SecureFormat(title, 256, L"Select a flow graph file");
		spec.lpstrTitle = title;

		spec.Flags = 0;

		char currentDirectory[_MAX_PATH];
		GetCurrentDirectoryA(_MAX_PATH, currentDirectory);

		bool isOpen = false;

		if (GetOpenFileNameW(&spec))
		{
			SetWindowTextW(hRoot, spec.lpstrFile);
			isOpen = true;
		}

		SetCurrentDirectoryA(currentDirectory);

		return isOpen;
	}

	void SetTitleWithFilename(IAbstractEditorSupervisor& editor, const wchar_t* filePath)
	{
		auto& super = static_cast<Abedit::IWin32AbstractEditorSupervisor&>(editor);
		super.SetTitleWithPath(GetCFGSAppTitle(), filePath);
	}
}

