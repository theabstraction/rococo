#pragma once

#ifndef INCLUDED_STRINGS_REFLECTION
# define INCLUDED_STRINGS_REFLECTION
#endif

#ifndef INCLUDED_ROCOCO_STRINGS
# include <rococo.strings.h>
#endif

#ifndef INCLUDED_ROCOCO_REFLECTOR
# include <rococo.reflector.h>
#endif

namespace Rococo::Strings
{
	using namespace Rococo::Reflection;

	struct Reflected_HString : public IReflectedString
	{
		HString& src;

		Reflected_HString(HString& _src) : src(_src)
		{

		}

		operator IReflectedString& () { return *this; }

		int32 Length() const override
		{
			return src.to_fstring().length;
		}

		cstr ReadString() const override
		{
			return src.c_str();
		}

		void WriteString(cstr s) override
		{
			src = s;
		}
	};

	Reflected_HString Reflect(HString& a)
	{
		return Reflected_HString(a);
	}
}