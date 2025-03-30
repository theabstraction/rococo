#pragma once

#include <rococo.types.h>

#ifndef INCLUDED_STRINGS_REFLECTION
# define INCLUDED_STRINGS_REFLECTION
#endif

#ifndef INCLUDED_ROCOCO_STRINGS
# include <rococo.strings.h>
#endif

#ifndef INCLUDED_ROCOCO_REFLECTOR
# include <rococo.reflector.h>
#endif

namespace Rococo::Reflection
{
	ROCOCO_API void ReflectStackFormat(IReflectionVisitor& v, cstr variableName, const char* format, ...);
	ROCOCO_API void EnterSection(IReflectionVisitor& v, const char* format, ...);
	ROCOCO_API void EnterElement(IReflectionVisitor& v, const char* format, ...);
}