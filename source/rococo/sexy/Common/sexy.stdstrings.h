#ifndef SEXY_STDSTRINGS_H
#define SEXY_STDSTRINGS_H

#include <string>

#include <rococo.strings.h>
#include <rococo.stl.allocators.h>

namespace Rococo
{
#ifdef char_IS_WIDE
	typedef std::wstring stdstring;
#else
	typedef rstdstring stdstring;
#endif
}

#endif // SEXY_STDSTRINGS_H