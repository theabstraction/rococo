#ifndef SEXY_STDSTRINGS_H
#define SEXY_STDSTRINGS_H

#include <string>

namespace Rococo
{
#ifdef SEXCHAR_IS_WIDE
	typedef std::wstring stdstring;
#else
	typedef std::string stdstring;
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