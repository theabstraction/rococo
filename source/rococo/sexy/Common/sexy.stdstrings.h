#ifndef SEXY_STDSTRINGS_H
#define SEXY_STDSTRINGS_H

#include <string>

#include <rococo.strings.h>
#include <rococo.stl.allocators.h>

namespace Rococo
{
	using rstdstring = std::basic_string<char, std::char_traits<char>, Rococo::Memory::SexyAllocator<char>>;
	using stdstring = rstdstring;
}

#endif // SEXY_STDSTRINGS_H