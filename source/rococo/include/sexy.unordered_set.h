#pragma once
#include <rococo.stl.allocators.h>
#include <unordered_set>

namespace Rococo
{
	template<typename KEY_, typename HASHER_ = std::hash<KEY_>, typename EQUAL_ = std::equal_to<KEY_>>
	using TSexyHashSet = std::unordered_set<KEY_, HASHER_, EQUAL_, Rococo::Memory::SexyAllocator<KEY_>>;
}