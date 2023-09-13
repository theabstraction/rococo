#define ROCOCO_COMPONENTS_API __declspec(dllexport)
#define ROCOCO_COMPONENTS_ANIMATION_API ROCOCO_COMPONENTS_API
#include <components/rococo.component.scripting.h>
#include <components/rococo.components.animation.h>
#include <..\component.modules\animation\code-gen\animation.sxh.h>

namespace
{
	Rococo::Components::Generated::IAnimationBase* FactoryConstructRococoComponentsAnimationGetAnimation(Rococo::Components::Generated::IAnimationBase* base)
	{
		return base;
	}
}

#include <..\component.modules\animation\code-gen\animation.sxh.inl>

PUBLISH_NATIVE_CALLS(Animation, AddNativeCalls_RococoComponentsGeneratedIAnimationBase)
