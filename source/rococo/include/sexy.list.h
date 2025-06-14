#pragma once
#include <rococo.sexy.allocators.h>
#include <list>

namespace Rococo
{
	template<typename VALUE_>
	using TSexyList = std::list<VALUE_, Rococo::Memory::SexyAllocator<VALUE_>>;
}