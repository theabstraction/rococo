#pragma once
#include <rococo.types.h>

namespace Rococo::Components
{
	struct ComponentTypeInfo
	{
		cstr shortName;
	};

	struct ComponentReflectionInfo
	{
		int todo;
	};

	// Base from which all components are derived.
	ROCOCO_INTERFACE IComponentBase
	{
		virtual ComponentTypeInfo TypeInfo() const = 0;
		virtual void Reflect(ComponentReflectionInfo& info) = 0;
		virtual ~IComponentBase() {}
	};

	// Note:
	// The IComponentBase base must have at least one virtual function, otherwise static_cast<IComponentBase*>(&derived) != reinterpret_cast<IComponentBase*>(&derived).
	// In rococo.component.base.h we ensure this by asserting sizeof(base) == sizeof(derived)
}
