#pragma once

#include <rococo.types.h>

namespace Rococo::Reflection
{
	ROCOCO_INTERFACE IReflectionVisitor
	{

	};

	ROCOCO_INTERFACE IReflectionTarget
	{
		virtual void Visit(IReflectionVisitor& v) = 0;
	};
}