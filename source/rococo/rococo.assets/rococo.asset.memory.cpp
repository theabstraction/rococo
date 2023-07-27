#include <rococo.types.h>

#define ROCOCO_ASSETS_API ROCOCO_API_EXPORT
#include <rococo.assets.h>

#include <allocators/rococo.allocators.dll.inl>

DEFINE_DLL_IALLOCATOR(assetAllocator)
DEFINE_FACTORY_DLL_IALLOCATOR_AS_BLOCK(assetAllocator, 1024, AssetModule)

#include <allocators/rococo.allocators.inl>

DeclareAllocator(TrackingAllocator, AssetModule, g_allocator)
Rococo::Memory::AllocatorMonitor<AssetModule> monitor; // When the progam terminates this object is cleared up and triggers the allocator log
OVERRIDE_MODULE_ALLOCATORS_WITH_FUNCTOR(g_allocator)