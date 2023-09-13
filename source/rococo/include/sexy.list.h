#pragma once
#include <rococo.stl.allocators.h>
#include <list>

namespace Rococo
{
	template<typename VALUE_>
	using TSexyList = std::list<VALUE_, Rococo::Memory::SexyAllocator<VALUE_>>;
}