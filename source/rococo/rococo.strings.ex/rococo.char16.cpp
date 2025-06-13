#include <rococo.types.h>

// Matter of interest, the string view header requires 4000 lines of code to be compiled. 
// Compare with the ~22 lines in rococo.types.h for the definition of Rococo::Strings::Substring

#include <string_view> 

namespace Rococo::Strings
{
	ROCOCO_API const char16_t* FindSubstring(const char16_t* bigText, const char16_t* searchTerm)
	{
		auto view = std::u16string_view(bigText);
		auto i = view.find(searchTerm);
		return (i == std::u32string_view::npos) ? nullptr : view.substr(i).data();
	}

	// N.B sexy script language string length is int32 with max 2^31-1 chars
	ROCOCO_API int32 StringLength(const char16_t* s)
	{
		if (s == nullptr)
		{
			Throw(0, "StringLength(Null string)");
		}

		auto view = std::u16string_view(s);
		size_t len = view.length();
		if (len > (0x7FFFFFFFULL))
		{
			Throw(0, "StringLength(string too long at %llu chars)", len);
		}

		return (int32) len;
	}

	ROCOCO_API void StringCat(char16_t* buf, const char16_t* source, int maxChars)
	{
		if (buf == nullptr)
		{
			Throw(0, "StringCat(buf was null)");
		}

		if (source == nullptr)
		{
			Throw(0, "StringCat(source was null)");
		}

		if (maxChars < 1)
		{
			return;
		}

		char16_t* dest = buf;
		char16_t* pos = dest + StringLength(buf);

		for (auto src = source; *src != 0; src++, pos++)
		{
			if (pos - buf >= maxChars)
			{
				buf[maxChars - 1] = 0;
				return;
			}

			*pos = *src;
		}

		*pos = 0;
	}
}