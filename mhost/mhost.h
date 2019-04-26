#pragma once

#include <rococo.api.h>
#include <rococo.renderer.h>

using namespace Rococo;

#include "mhost.sxh.h"

namespace MHost
{
	using namespace Rococo;

	// Returns the top left position, using alignment flags to interpret how the pos argument is interpreted
	Vec2 GetTopLeftPos(Vec2 pos, Vec2 span, int32 alignmentFlags);
}