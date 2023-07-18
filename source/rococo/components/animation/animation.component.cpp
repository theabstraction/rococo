#define ROCOCO_COMPONENTS_ANIMATION_API __declspec(dllexport)
#include <components/rococo.components.animation.h>
#include <rococo.animation.h>
#include <rococo.ecs.builder.inl>
#include <rococo.allocators.inl>

using namespace Rococo::Memory;

DeclareAllocator(TrackingAllocator, Animation, g_allocator);
Rococo::Memory::AllocatorMonitor<Animation> monitor;

OVERRIDE_MODULE_ALLOCATORS_WITH_FUNCTOR(g_allocator)

namespace Rococo::Components
{
	using namespace Rococo::Entities;

	struct AnimationComponent : IAnimationComponent
	{
		AutoFree<IAnimation> animation;
		AnimationComponent():
			animation(CreateAnimation())
		{

		}

		Rococo::Entities::IAnimation& Core() override
		{
			return *animation;
		}
	};
}

DEFINE_AND_EXPORT_SINGLETON_METHODS_WITH_DEFAULT_FACTORY(ROCOCO_COMPONENTS_ANIMATION_API, IAnimationComponent, AnimationComponent)
