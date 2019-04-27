#pragma once

#include <rococo.api.h>
#include <rococo.renderer.h>

using namespace Rococo;

namespace MHost 
{
	namespace OS
	{
		struct KeyState
		{
			uint8 keys[256];
		};
	}
}

namespace MHost
{
	struct IEngineBase
	{
		virtual void Free() = 0;
	};

	struct IEngine;
}

#include "mhost.sxh.h"

namespace MHost
{
	using namespace Rococo;

	// Returns the top left position, using alignment flags to interpret how the pos argument is interpreted
	Vec2 GetTopLeftPos(Vec2 pos, Vec2 span, int32 alignmentFlags);
}