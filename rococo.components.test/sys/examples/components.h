#pragma once

#include <rococo.types.h>

namespace Rococo::Components
{
	ROCOCOAPI IFireComponent
	{
		virtual void Burn() = 0;
	};

	ROCOCOAPI IWaterComponent
	{
		virtual void Flood() = 0;
	};
}
