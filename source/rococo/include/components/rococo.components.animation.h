#pragma once
#include <rococo.ecs.ex.h>

namespace Rococo::Entities
{
	struct IAnimation;
}

namespace Rococo::Components
{
	ROCOCO_INTERFACE IAnimationComponent : IComponentBase
	{
		virtual Rococo::Entities::IAnimation & Core() = 0;
		virtual ~IAnimationComponent() {}
	};
}

#ifndef ROCOCO_COMPONENTS_ANIMATION_API
# define ROCOCO_COMPONENTS_ANIMATION_API ROCOCO_API_IMPORT
#endif

DECLARE_SINGLETON_METHODS(ROCOCO_COMPONENTS_ANIMATION_API, IAnimationComponent)