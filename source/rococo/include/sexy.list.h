// Copyright (c)2025 Mark Anthony Taylor. Email: mark.anthony.taylor@gmail.com. All rights reserved.
#pragma once
#include <rococo.sexy.allocators.h>
#include <list>

namespace Rococo
{
	template<typename VALUE_>
	using TSexyList = std::list<VALUE_, Rococo::Memory::SexyAllocator<VALUE_>>;
}