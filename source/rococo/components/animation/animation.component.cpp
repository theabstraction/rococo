#define ROCOCO_COMPONENTS_ANIMATION_API __declspec(dllexport)
#include <components/rococo.components.animation.h>
#include <rococo.animation.h>
#include <rococo.ecs.builder.inl>

//#include <rococo.allocators.trackers.inl>

#include <allocators/rococo.allocators.dll.inl>

DEFINE_DLL_IALLOCATOR(g_iAllocator)
DEFINE_FACTORY_DLL_IALLOCATOR_AS_BLOCK(g_iAllocator, 128, AnimationModule)

#include <allocators/rococo.allocators.inl>

DeclareAllocator(TrackingAllocator, AnimationModule, g_allocator)
Rococo::Memory::AllocatorMonitor<AnimationModule> monitor; // When the progam terminates this object is cleared up and triggers the allocator log
OVERRIDE_MODULE_ALLOCATORS_WITH_FUNCTOR(g_allocator)

DEFINE_FACTORY_SINGLETON(IAnimationComponent)
EXPORT_SINGLETON_METHODS(ROCOCO_COMPONENTS_ANIMATION_API, IAnimationComponent)
SINGLETON_MANAGER(ROCOCO_COMPONENTS_ANIMATION_API, IAnimationComponent)

