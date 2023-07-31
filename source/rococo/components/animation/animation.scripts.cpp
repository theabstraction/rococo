#define ROCOCO_COMPONENTS_ANIMATION_API __declspec(dllexport)

#include <rococo.ecs.h>
#include <sexy.script.h>

#include <..\components\animation\code-gen\rococo.animation.sxh.h>

namespace
{
	Rococo::Components::Animation::IAnimationBase* FactoryConstructRococoComponentsAnimationGetAnimation(Rococo::Components::Animation::IAnimationBase* base)
	{
		return base;
	}
}

#include <..\components\animation\code-gen\rococo.animation.sxh.inl>

namespace Rococo::Components::Animation
{
	ROCOCO_COMPONENTS_ANIMATION_API void AddNativeCalls(Rococo::Script::IPublicScriptSystem& ss, Rococo::Components::Animation::IAnimationBase* base)
	{
		AddNativeCalls_RococoComponentsAnimationIAnimationBase(ss, base);
	}
}