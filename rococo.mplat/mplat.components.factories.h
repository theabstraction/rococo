#pragma once

#include "mplat.components.decl.h"

namespace Rococo::Components
{
	template<class ICOMPONENT>
	ROCOCOAPI IComponentFactory
	{
		virtual ICOMPONENT* ConstructInPlace(void* pMemory) = 0;
		virtual void Destruct(ICOMPONENT* pInstance) = 0;
		virtual size_t SizeOfConstructedObject() const = 0;
	};
}

namespace Rococo::Components::Sys
{
	ROCOCOAPI IParticleSystemComponentFactory : IComponentFactory<IParticleSystemComponent>{};
	ROCOCOAPI IRigsComponentFactory : IComponentFactory<IRigsComponent>{};
	ROCOCOAPI IEntityFactory: IComponentFactory<IEntity>{};
}

