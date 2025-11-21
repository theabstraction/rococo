#pragma once
#include <rococo.types.h>

namespace Rococo::Strings
{
	DECLARE_ROCOCO_INTERFACE StringBuilder;
}

namespace Rococo::Unreal
{
	ROCOCO_INTERFACE IUnrealArg
	{
		virtual void AppendName(Strings::StringBuilder & sb) const = 0;
		virtual void AppendType(Strings::StringBuilder& sb) const = 0;
		virtual bool IsConst() const = 0;
		virtual bool IsRef() const = 0;
	};

	ROCOCO_INTERFACE IUnrealFunction
	{
		virtual void AppendFunctionName(Strings::StringBuilder& sb) const = 0;
		virtual IUnrealArg* GetArg(size_t index) = 0;
	};

	ROCOCO_INTERFACE IUnrealClass
	{
		virtual cstr ShortName() const = 0;
		virtual size_t MethodCount() const = 0;
		virtual IUnrealFunction& GetFunction(size_t index) = 0;
	};
}