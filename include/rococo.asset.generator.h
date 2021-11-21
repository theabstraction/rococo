#pragma once

#include <rococo.api.h>

namespace Rococo::Script
{
	struct IPublicScriptSystem;
}

namespace Rococo::Assets
{
	ROCOCOAPI IAssetBuilder
	{
		virtual void AppendHeader(cstr name, cstr typename, cstr moduleName) = 0;
		virtual void AppendValue(int32 value) = 0;
		virtual void AppendValue(int64 value) = 0;
		virtual void AppendValue(float value) = 0;
		virtual void AppendValue(double value) = 0;
		virtual void AppendValue(bool value) = 0;

		virtual void EnterMembers(cstr name, cstr typename, cstr moduleName) = 0;
		virtual void LeaveMembers() = 0;

		virtual void Free() = 0;
	};

	ROCOCOAPI IAssetGenerator
	{
		virtual IAssetBuilder* CreateAssetBuilder(const fstring& pingPath) = 0;
		virtual void Free() = 0;
	};

	void LinkAssetGenerator(IAssetGenerator& generator, Rococo::Script::IPublicScriptSystem& ss);

	IAssetGenerator* CreateAssetGenerator_CSV(IInstallation& installation, bool addHumanReadableReferences);
}
