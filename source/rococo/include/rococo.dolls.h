#pragma once
#include <rococo.types.h>

#ifndef ROCOCO_DOLLS_API
# define ROCOCO_DOLLS_API ROCOCO_API_IMPORT
#endif

namespace Rococo::Dolls
{
	ROCOCO_INTERFACE IDollLoader
	{

	};

	ROCOCO_INTERFACE IDollLoaderSupervisor : IDollLoader
	{
		virtual void Free() = 0;
	};

	ROCOCO_DOLLS_API IDollLoaderSupervisor* CreateDollLoader();
}