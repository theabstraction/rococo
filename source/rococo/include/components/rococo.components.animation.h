#pragma once
#include <components/rococo.ecs.ex.h>

#include <rococo.animation.h>
#include "../../component.modules/animation/code-gen/animation.sxh.h"

namespace Rococo::Components
{
	// An animation components manages key animation frames associated with a ROID
	ROCOCO_INTERFACE IAnimationComponent : Rococo::Components::Generated::IAnimationBase
	{
		virtual void Advance(Rococo::Entities::AnimationAdvanceArgs& args) = 0;
	};
}

#ifndef ROCOCO_COMPONENTS_ANIMATION_API
# define ROCOCO_COMPONENTS_ANIMATION_API ROCOCO_API_IMPORT
#endif

DECLARE_SINGLETON_METHODS(ROCOCO_COMPONENTS_ANIMATION_API, IAnimationComponent)