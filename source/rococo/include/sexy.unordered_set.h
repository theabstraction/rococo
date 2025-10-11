// Copyright (c)2025 Mark Anthony Taylor. Email: mark.anthony.taylor@gmail.com. All rights reserved.
#pragma once
#include <rococo.sexy.allocators.h>
#include <unordered_set>

namespace Rococo
{
	template<typename KEY_, typename HASHER_ = std::hash<KEY_>, typename EQUAL_ = std::equal_to<KEY_>>
	using TSexyHashSet = std::unordered_set<KEY_, HASHER_, EQUAL_, Rococo::Memory::SexyAllocator<KEY_>>;
}