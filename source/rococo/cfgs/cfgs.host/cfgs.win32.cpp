#include <rococo.os.win32.global-ns.h>
#include <rococo.window.h>
#include <rococo.editors.h>
#include <rococo.abstract.editor.h>
#include <rococo.strings.h>
#include <commdlg.h>
#include <stdlib.h>
#include <rococo.cfgs.h>

using namespace Rococo::Abedit;

namespace Rococo::CFGS
{
	Editors::IUI2DGridSlateSupervisor* Create2DGridControl(Abedit::IAbstractEditorSupervisor& editor, Rococo::Editors::IUI2DGridEvents& eventHandler)
	{
		if (!Strings::Eq(editor.Implementation(), IMPLEMENTATION_TYPE_WIN32_HWND))
		{
			Throw(0, "%s: implementation needs to be %s", __ROCOCO_FUNCTION__, IMPLEMENTATION_TYPE_WIN32_HWND);
		}

		auto& super = static_cast<Abedit::IWin32AbstractEditorSupervisor&>(editor);
		return Rococo::Windows::Create2DGrid(super.Slate(), WS_VISIBLE | WS_CHILD, eventHandler, true);
	}

	typedef ICFGSIntegratedDevelopmentEnvironmentSupervisor* (*FN_Create_CFGS_Win32_IDE)(HWND hHostWindow, ICFGSDatabase& db, Rococo::Abedit::IAbstractEditor& editor, Rococo::Events::IPublisher& publisher, ICFGSControllerConfig& config);

	ICFGSIntegratedDevelopmentEnvironmentSupervisor* Create_CFGS_IDE(IAbstractEditorSupervisor& editor, ICFGSDatabase& db, Rococo::Events::IPublisher& publisher, ICFGSControllerConfig& config)
	{
		auto& super = static_cast<Abedit::IWin32AbstractEditorSupervisor&>(editor);
		HWND hRoot = GetAncestor(super.Slate(), GA_ROOT);

		cstr dllname = "cfgs.sexy.ide.dll";

		HMODULE hPopupModule = LoadLibraryA(dllname);
		if (!hPopupModule)
		{
			Throw(GetLastError(), "%s: Could not load library: %s", __ROCOCO_FUNCTION__, dllname);
		}

		cstr procName = "Create_CFGS_Win32_IDE";

		FARPROC factoryProc = GetProcAddress(hPopupModule, procName);
		if (!factoryProc)
		{
			Throw(GetLastError(), "%s: Could not load find '%s' in %s", __ROCOCO_FUNCTION__, procName, dllname);
		}

		FN_Create_CFGS_Win32_IDE createIDE = (FN_Create_CFGS_Win32_IDE)factoryProc;

		return createIDE(hRoot, db, editor, publisher, config);
	}

	bool TryGetUserSelectedCFGSPath(OUT U8FilePath& path, Abedit::IAbstractEditorSupervisor& editor)
	{
		auto& super = static_cast<Abedit::IWin32AbstractEditorSupervisor&>(editor);
		HWND hRoot = GetAncestor(super.Slate(), GA_ROOT);

		OPENFILENAMEA spec = { 0 };
		spec.lStructSize = sizeof(spec);
		spec.hwndOwner = hRoot;
		spec.lpstrFilter = "control-flow graph SXML file\0*.cfgs.sexml\0\0";
		spec.nFilterIndex = 1;

		spec.lpstrFile = path.buf;
		*path.buf = 0;
		spec.nMaxFile = sizeof(path) / sizeof(wchar_t);

		char title[256];
		Strings::SecureFormat(title, 256, "Select a flow graph file");
		spec.lpstrTitle = title;

		spec.Flags = 0;

		char currentDirectory[_MAX_PATH];
		GetCurrentDirectoryA(_MAX_PATH, currentDirectory);

		bool isOpen = false;

		// This outputs: onecore\vm\dv\storage\plan9\rdr\dll\util.cpp(99)\p9np.dll. No idea why. Maybe one day it will go away
		if (GetOpenFileNameA(&spec))
		{
			SetWindowTextA(hRoot, spec.lpstrFile);
			isOpen = true;
		}

		SetCurrentDirectoryA(currentDirectory);

		return isOpen;
	}

	bool TryGetUserCFGSSavePath(OUT U8FilePath& path, Abedit::IAbstractEditorSupervisor& editor)
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
		spec.nMaxFile = U8FilePath::CAPACITY;

		char title[256];
		Strings::SecureFormat(title, 256, "Save a flow graph file");
		spec.lpstrTitle = title;

		spec.Flags = 0;

		char currentDirectory[_MAX_PATH];
		GetCurrentDirectoryA(_MAX_PATH, currentDirectory);

		bool isSaved = false;

		if (GetSaveFileNameA(&spec))
		{
			SetWindowTextA(hRoot, spec.lpstrFile);
			isSaved = true;
		}

		SetCurrentDirectoryA(currentDirectory);

		return isSaved;
	}

	void SetTitleWithFilename(IAbstractEditorSupervisor& editor, cstr filePath)
	{
		auto& super = static_cast<Abedit::IWin32AbstractEditorSupervisor&>(editor);
		super.SetTitleWithPath(GetCFGSAppTitle(), filePath);
	}
}

