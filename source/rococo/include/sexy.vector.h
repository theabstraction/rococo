#pragma once
#include <rococo.stl.allocators.h>
#include <vector>

namespace Rococo
{
	template<typename VALUE_>
	using TSexyVector = std::vector<VALUE_, Rococo::Memory::SexyAllocator<VALUE_>>;
}