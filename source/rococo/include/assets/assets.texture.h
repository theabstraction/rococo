#pragma once
#include <rococo.assets.h>

namespace Rococo::IO
{
	struct IInstallation;
}

namespace Rococo::Assets
{
	ROCOCO_INTERFACE IFileAsset: IAsset
	{

	};

	ROCOCO_INTERFACE IFileAssetFactory
	{
		virtual IFileAsset * CreateFileAsset(const char* utf8Path) = 0;
	};

	ROCOCO_INTERFACE IFileAssetFactorySupervisor: IFileAssetFactory
	{
		virtual void Free() = 0;
	};

	IFileAssetFactorySupervisor* CreateFileAssetFactory(IAssetManager& manager, IO::IInstallation& installation);
}