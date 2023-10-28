#pragma once

#include <rococo.types.h>

namespace Rococo
{
	inline void operator += (Seconds& s, Seconds dt)
	{
		s.value += dt.value;
	}
}