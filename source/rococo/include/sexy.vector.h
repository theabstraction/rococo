#pragma once
#include <rococo.sexy.allocators.h>
#include <vector>

namespace Rococo
{
	template<typename VALUE_>
	using TSexyVector = std::vector<VALUE_, Rococo::Memory::SexyAllocator<VALUE_>>;
}