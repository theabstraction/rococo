#ifndef SEXY_STDSTRINGS_H
#define SEXY_STDSTRINGS_H

#include <string>
#include <sstream>

namespace Sexy
{
#ifdef SEXCHAR_IS_WIDE
	typedef std::wstring stdstring;
	typedef std::wstringstream sexstringstream;
#else
	typedef std::string stdstring;
	typedef std::stringstream std::sexstringstream;
#endif
}

#endif // SEXY_STDSTRINGS_H