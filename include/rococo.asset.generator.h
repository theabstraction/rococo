#pragma once

#include <rococo.api.h>

namespace Rococo::Script
{
	struct IPublicScriptSystem;
}

namespace Rococo::Sexy 
{
	struct IAssetLoader;
}

namespace Rococo::Assets
{
	ROCOCOAPI IAssetBuilder
	{
		virtual void AppendHeader(cstr name, cstr typename, cstr moduleName) = 0;
		virtual void AppendInterfaceType(cstr interfaceType, cstr name, cstr moduleName) = 0;
		virtual void AppendObjectRef(cstr type, cstr moduleName, cstr objectValueName) = 0;
		virtual void AppendStringConstant(cstr name, cstr buffer, int32 length) = 0;
		virtual void AppendValue(cstr fieldName, int32 value) = 0;
		virtual void AppendValue(cstr fieldName, int64 value) = 0;
		virtual void AppendValue(cstr fieldName, float value) = 0;
		virtual void AppendValue(cstr fieldName, double value) = 0;
		virtual void AppendValue(cstr fieldName, bool value) = 0;

		virtual void EnterMembers(cstr name, cstr typename, cstr moduleName) = 0;
		virtual void LeaveMembers() = 0;

		virtual void Free() = 0;
	};

	ROCOCOAPI IAssetGenerator
	{
		virtual IAssetBuilder* CreateAssetBuilder(const fstring& pingPath) = 0;
		virtual void Free() = 0;
	};

	// Enables Sexy semantic:
	// (reflect SaveAsset <SexyFileAsset-variable> <target-object>) 
	// to serialize a target-object to the supplied asset generator
	void LinkAssetGenerator(IAssetGenerator& generator, Rococo::Script::IPublicScriptSystem& ss);

	// Enables Sexy semantic:
	// (reflect LoadAsset <SexyFileAsset-variable> <target-object>) 
	// to serialize a target-object from the supplied asset loader
	void LinkAssetLoader(Rococo::Sexy::IAssetLoader& loader, Rococo::Script::IPublicScriptSystem& ss);

	// Create a CSV asset generator. These allow creation of files that save objects in CSV format.
	IAssetGenerator* CreateAssetGenerator_CSV(IInstallation& installation, bool addHumanReadableReferences);
}
