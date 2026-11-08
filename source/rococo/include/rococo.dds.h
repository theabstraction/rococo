#pragma once

#include <rococo.types.h>

#ifndef ROCOCO_DDS_API
# define ROCOCO_DDS_API ROCOCO_API_IMPORT
#endif

namespace Rococo
{
	namespace IO
	{
		DECLARE_ROCOCO_INTERFACE IInstallation;
	}

	namespace Graphics
	{
		struct DDSContext;
		DECLARE_ROCOCO_INTERFACE IDDSSysTexture;
	}

	ROCOCO_INTERFACE IDDSTexture
	{
		virtual Graphics::IDDSSysTexture& SysTexture() = 0;
	};

	ROCOCO_INTERFACE IDDSTextureSupervisor: IDDSTexture
	{
		virtual void Free() = 0;
	};

	ROCOCO_INTERFACE IDDSLoader
	{
		virtual IDDSTextureSupervisor* Load(cstr pingPath, Graphics::DDSContext& context) = 0;
	};

	ROCOCO_INTERFACE IDDSLoaderSupervisor : IDDSLoader
	{
		virtual void Free() = 0;
	};

	// MaxTxSize should be a power of two and gives the max texture span in loading operations
	ROCOCO_DDS_API IDDSLoaderSupervisor* CreateDDSSystem(IO::IInstallation& installation, size_t _maxTxSize = 2048);
}