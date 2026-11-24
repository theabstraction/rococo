// Copyright (c)2025 Mark Anthony Taylor. Email: mark.anthony.taylor@gmail.com. All rights reserved.
#pragma once

#include "sexystudio.database.h"

namespace Rococo::SexyStudio
{
	ROCOCO_INTERFACE IWorkspaceObserver
	{
		// Called when the workspace selects another project, or repopulates the database of the current project. Also called once by ISexyWorkspace::AddObserver,
		// so an observer added late still shows the current state. [current] is the database the workspace now presents.
		// [contentIsNew] is true when the database was (re)populated since the observer last heard about it, which includes the call from AddObserver when
		// a project is selected. It is false when the workspace switches back to a project already loaded, or when there is no project.
		// An observer should treat the database as read-only, and must not add or remove observers in the callback.
		virtual void OnWorkspaceChanged(ISexyDatabase& current, bool contentIsNew) = 0;
	};

	ROCOCO_INTERFACE IWorkspaceErrorHandler
	{
		// [context] names what failed, such as the path of a project config that could not be saved
		virtual void OnWorkspaceError(cstr context, IException& ex) = 0;
	};

	// The SexyStudio workspace: it owns the projects, which are the sexystudio.config.sexml files with their search paths and package selections, and a database
	// for each. It decides which project is current and when a database is populated. Each project is loaded and populated once, when it is first selected,
	// and thereafter only when Repopulate is called. Without a project the workspace presents a blank database, with no search paths and no packages.
	ROCOCO_INTERFACE ISexyWorkspace
	{
		// The database of the current project, or the blank database if there is none
		virtual ISexyDatabase& GetDatabase() = 0;
		virtual const ISexyDatabase& GetDatabase() const = 0;

		// True if and only if a project is selected
		[[nodiscard]] virtual bool HasProject() const = 0;

		// Selects the project whose config is <sysPathToConfigDirectory>sexystudio.config.sexml, loading and populating it if this is the first time.
		// The directory must end with a directory separator. Throws if there is no such config file
		virtual void SetDirectoryContainingConfigFile(cstr sysPathToConfigDirectory) = 0;

		// As SetDirectoryContainingConfigFile, but a project already loaded is repopulated, so changes on disk are picked up. This is for a path the user entered
		virtual void ReloadProjectByConfigDirectory(cstr sysPathToConfigDirectory) = 0;

		// Selects the project that contains the given .sxy file, loading it if necessary. Paths that do not end in .sxy are ignored
		virtual void Rebase(crwstr fullPathToSxyFile) = 0;

		// Clears and repopulates the current project's database from its search paths and packages, then tells the observers. Does nothing without a project
		virtual void Repopulate() = 0;

		virtual void AddObserver(IWorkspaceObserver& observer) = 0;
		virtual void RemoveObserver(IWorkspaceObserver& observer) = 0;

		// Selects the project named in the user's defaults file, the section given to CreateSexyWorkspace. A missing or invalid defaults file is ignored
		virtual void LoadDefaults() = 0;

		// Records the current project's config path in the user's defaults file. Does nothing without a project. Errors are ignored, as the defaults are a convenience
		virtual void SaveDefaults() = 0;

		// Writes each project's config back to its sexystudio.config.sexml. A project that cannot be saved is skipped and onError, if not null, is told why
		virtual void SaveProjects(IWorkspaceErrorHandler* onError) = 0;

		virtual void Free() = 0;
	};

	// Creates an empty workspace. [defaultsSection] names the user SEXML file that LoadDefaults and SaveDefaults use, e.g. "SexyStudio.Defaults".
	// Nothing is loaded or saved until the caller asks
	SEXYSTUDIO_API ISexyWorkspace* CreateSexyWorkspace(cstr defaultsSection);
}
