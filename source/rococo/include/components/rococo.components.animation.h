#pragma once
#include <rococo.ecs.h>

#ifndef ROCOCO_COMPONENTS_ANIMATION_API
# define ROCOCO_COMPONENTS_ANIMATION_API ROCOCO_API_IMPORT
#endif

namespace Rococo::Entities
{
	struct IAnimation;
}

namespace Rococo::Components
{
	ROCOCO_INTERFACE IAnimationComponent : IComponentBase
	{
		virtual Rococo::Entities::IAnimation& Core() = 0;
	};

	ROCOCO_COMPONENTS_ANIMATION_API Ref<IAnimationComponent> AddAnimationComponent(ROID id);
	ROCOCO_COMPONENTS_ANIMATION_API Ref<IAnimationComponent> GetAnimationComponent(ROID id);
	ROCOCO_COMPONENTS_ANIMATION_API void AnimationComponent_LinkToECS(IECS& ecs);
}