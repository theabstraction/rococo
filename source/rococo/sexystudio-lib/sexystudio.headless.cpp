// Copyright (c)2025 Mark Anthony Taylor. Email: mark.anthony.taylor@gmail.com. All rights reserved.

// CreateHeadlessSexyStudio: a workspace and a completion engine with no window, for tests, tools and editors that do not want the IDE

#include "sexystudio.api.h"
#include "sexystudio.engine.h"
#include <rococo.strings.h>
#include <rococo.auto-release.h>

using namespace Rococo;
using namespace Rococo::SexyStudio;

namespace
{
	// Used when the spec gives no host. File changes need no action without a window, and an error is thrown out of the gaffer method that hit it
	struct RethrowingEngineHost final : IEngineHost
	{
		void OnEditorFileChanged(crwstr fullPath) override
		{
			UNUSED(fullPath);
		}

		void OnEngineError(IException& ex) override
		{
			Throw(ex.ErrorCode(), "%s", ex.Message());
		}
	};

	struct HeadlessSexyStudio final : ISexyStudioHeadless
	{
		RethrowingEngineHost defaultHost;
		AutoFree<ISexyWorkspace> workspace;
		AutoFree<ISexyCompletionEngine> engine;

		HeadlessSexyStudio(const HeadlessSexyStudioSpec& spec) :
			workspace(CreateSexyWorkspace(spec.defaultsSection ? spec.defaultsSection : "SexyStudio.Defaults")),
			engine(CreateSexyCompletionEngine(*workspace, spec.host ? *spec.host : static_cast<IEngineHost&>(defaultHost)))
		{
			if (spec.loadDefaults)
			{
				workspace->LoadDefaults();
			}
		}

		ISexyDatabase& GetDatabase() override
		{
			return workspace->GetDatabase();
		}

		const ISexyDatabase& GetDatabase() const override
		{
			return workspace->GetDatabase();
		}

		void SetDirectoryContainingConfigFile(cstr sysPathToConfigDirectory) override
		{
			workspace->SetDirectoryContainingConfigFile(sysPathToConfigDirectory);
		}

		ISexyWorkspace& Workspace() override
		{
			return *workspace;
		}

		ISexyStudioCompletionGaffer& Gaffer() override
		{
			return *engine;
		}

		void Free() override
		{
			delete this;
		}
	};
}

namespace Rococo::SexyStudio
{
	SEXYSTUDIO_API ISexyStudioHeadless* CreateHeadlessSexyStudio(const HeadlessSexyStudioSpec& spec)
	{
		return new HeadlessSexyStudio(spec);
	}
}
