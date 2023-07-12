#pragma once

#include <rococo.api.h>

namespace Rococo::Script
{
	struct IPublicScriptSystem;
}

namespace Rococo::Assets
{
	ROCOCO_INTERFACE IAssetGenerator
	{
		// Obtain a string builder for building strings, specific to this class instance.
		virtual Strings::StringBuilder& GetReusableStringBuilder() = 0;
		virtual void Generate(cstr id, const fstring& stringSerialziation) = 0;
		virtual void Free() = 0;
	};

	// Enables Sexy semantic:
	// (reflect SaveAsset <SexyFileAsset-variable> <target-object>) 
	// to serialize a target-object to the supplied asset generator
	void LinkAssetGenerator(IAssetGenerator& generator, Rococo::Script::IPublicScriptSystem& ss);

	// Enables Sexy semantic:
	// (reflect LoadAsset <SexyFileAsset-variable> <target-object>) 
	// to serialize a target-object from the supplied asset loader
	void LinkAssetLoader(IO::IInstallation& installation, Rococo::Script::IPublicScriptSystem& ss);

	// Create an asset file generator, the target being a file saved to a Sexy content directory
	IAssetGenerator* CreateAssetGenerator_SexyContentFile(IO::IInstallation& installation);
}
