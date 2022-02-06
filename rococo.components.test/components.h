#pragma once

#include <rococo.types.h>

namespace Rococo::Components::Sys
{
	ROCOCOAPI IFireComponent
	{
		virtual void Burn() = 0;
		virtual bool Deprecate() = 0;
		virtual void Free() = 0;
		virtual bool IsReadyToDelete() const = 0;
	};

	// Base class for component factories, you should derive from this class and add a method 'virtual IMyComponent* ConstructInPlace(void* pMemory) = 0;'
	ROCOCOAPI IComponentFactory
	{
		// Number of bytes that represent the object. This should return sizeof(IComponentFactory implementation object)
		virtual size_t SizeOfConstructedObject() = 0;
	};

	ROCOCOAPI IFireComponentFactory : IComponentFactory
	{
		virtual IFireComponent* ConstructInPlace(void* pMemory) = 0;
	};
}
