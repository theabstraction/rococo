// Copyright (c)2025 Mark Anthony Taylor. Email: mark.anthony.taylor@gmail.com. All rights reserved.
#pragma once

// The SexyStudio completion engine and the windowless SexyStudio factory. Part of the GUI-free layer: the engine works on an AutoComplete::ISexyEditor
// and the workspace's database only. It must not include or forward declare window, widget, IDE or Notepad++ types

#include "sexystudio.workspace.h"
#include <rococo.sexystudio.engine.h>

namespace Rococo::SexyStudio
{
	// What the completion engine asks of whoever hosts it. The engine has no window: the IDE hides its frame in OnEditorFileChanged and shows
	// a message box in OnEngineError, as it always did. A headless host may record, ignore or rethrow
	ROCOCO_INTERFACE IEngineHost
	{
		// UpdateAutoComplete was given a file path that differs from the last one, and the workspace has rebased to the project of that file
		virtual void OnEditorFileChanged(crwstr fullPath) = 0;

		// Completion failed, for example the rebase threw. The completion is abandoned after this returns
		virtual void OnEngineError(IException& ex) = 0;
	};

	// The completion engine: ISexyStudioCompletionGaffer on the current database of a workspace
	ROCOCO_INTERFACE ISexyCompletionEngine : ISexyStudioCompletionGaffer
	{
		virtual void Free() = 0;
	};

	// Creates a completion engine on the workspace. Both the workspace and the host must outlive the engine
	SEXYSTUDIO_API ISexyCompletionEngine* CreateSexyCompletionEngine(ISexyWorkspace& workspace, IEngineHost& host);

	// SexyStudio without a window: a workspace, its databases and a completion engine on it. For tools, tests and editors that need completion but not the IDE
	ROCOCO_INTERFACE ISexyStudioHeadless : ISexyDatabaseSet
	{
		virtual ISexyWorkspace& Workspace() = 0;
		virtual ISexyStudioCompletionGaffer& Gaffer() = 0;
		virtual void Free() = 0;
	};

	struct HeadlessSexyStudioSpec
	{
		// The user SEXML file the workspace keeps the last project in. See CreateSexyWorkspace
		cstr defaultsSection = "SexyStudio.Defaults";

		// If true the workspace selects the project named in the defaults file at creation
		bool loadDefaults = true;

		// Receives the engine's notifications and errors. If null, file changes are ignored and errors are thrown out of the gaffer methods
		IEngineHost* host = nullptr;
	};

	// Creates a windowless SexyStudio. Nothing is saved automatically: call Workspace().SaveDefaults() or SaveProjects() if you want that
	SEXYSTUDIO_API ISexyStudioHeadless* CreateHeadlessSexyStudio(const HeadlessSexyStudioSpec& spec);
}
