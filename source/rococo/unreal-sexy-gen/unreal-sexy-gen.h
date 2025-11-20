#pragma once
#include <rococo.types.h>

namespace Rococo::Unreal
{
	ROCOCO_INTERFACE IUnrealFunction
	{
		virtual cstr FunctionName() const = 0;
	};

	ROCOCO_INTERFACE IUnrealClass
	{
		virtual cstr ShortName() const = 0;
		virtual size_t MethodCount() const = 0;
		virtual IUnrealFunction& GetFunction(size_t index) = 0;
	};
}