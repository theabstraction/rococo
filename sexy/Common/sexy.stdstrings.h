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

   template<size_t CAPACITY> class sexstringstream
   {
   private:
      char buffer[CAPACITY];
   public:
      StackStringBuilder sb;
      sexstringstream(): sb(buffer, CAPACITY) {}
      operator const char* () const { return buffer; }
   };
}

#endif // SEXY_STDSTRINGS_H