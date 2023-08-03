#define ROCOCO_COMPONENTS_ANIMATION_API __declspec(dllexport)
#include <components/rococo.components.animation.h>
#include <rococo.ecs.h>
#include <sexy.script.h>

#include <..\components\animation\code-gen\animation.sxh.h>

namespace
{
	Rococo::Components::Generated::IAnimationBase* FactoryConstructRococoComponentsAnimationGetAnimation(Rococo::Components::Generated::IAnimationBase* base)
	{
		return base;
	}
}

#include <..\components\animation\code-gen\animation.sxh.inl>

namespace Rococo::Components::Generated
{
	ROCOCO_COMPONENTS_ANIMATION_API void AddNativeCalls(Rococo::Script::IPublicScriptSystem& ss, Rococo::Components::Generated::IAnimationBase* base)
	{
		Interop::AddNativeCalls_RococoComponentsGeneratedIAnimationBase(ss, base);
	}
}