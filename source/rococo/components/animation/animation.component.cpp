#define ROCOCO_COMPONENTS_ANIMATION_API __declspec(dllexport)
#include <components/rococo.components.animation.h>
#include <rococo.animation.h>
#include <rococo.ecs.builder.inl>

//#include <rococo.allocators.trackers.inl>

#include <rococo.allocators.dll.inl>

DEFINE_DLL_IALLOCATOR(g_iAllocator)
DEFINE_FACTORY_DLL_IALLOCATOR_AS_BLOCK(g_iAllocator, 128, AnimationModule)

#include <rococo.allocators.inl>

DeclareAllocator(TrackingAllocator, AnimationModule, g_allocator)
Rococo::Memory::AllocatorMonitor<AnimationModule> monitor; // When the progam terminates this object is cleared up and triggers the allocator log
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
