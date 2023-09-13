#pragma once

#include <rococo.types.h>

namespace Rococo::Components
{
	ROCOCO_INTERFACE IFireComponent
	{
		virtual void Burn() = 0;
	};

	ROCOCO_INTERFACE IWaterComponent
	{
		virtual void Flood() = 0;
	};
}
