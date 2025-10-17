// Copyright (c)2025 Mark Anthony Taylor. Email: mark.anthony.taylor@gmail.com. All rights reserved.
#pragma once

#include <rococo.types.h>

namespace Rococo
{
	inline void operator += (Seconds& s, Seconds dt)
	{
		s.value += dt.value;
	}
}