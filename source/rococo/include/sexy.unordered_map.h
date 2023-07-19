#pragma once
#include <rococo.stl.allocators.h>
#include <unordered_map>

namespace Rococo
{
	template<typename KEY_,  typename VALUE_, typename HASHER_ = std::hash<KEY_>, typename EQUAL_ = std::equal_to<KEY_>>
	using TSexyHashMap = std::unordered_map<KEY_, VALUE_, HASHER_, EQUAL_, Rococo::Memory::SexyAllocator<std::pair<const KEY_, VALUE_>>>;

#ifdef SEXY_STDSTRINGS_H // #include "sexy.stdstrings.h" first to access the following template
	template<typename VALUE_>
	using TSexyHashMapByStdString = TSexyHashMap<stdstring, VALUE_, std::hash<stdstring>, std::equal_to<stdstring>>;
#endif
}