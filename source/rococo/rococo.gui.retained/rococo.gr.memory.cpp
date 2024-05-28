#include <allocators/rococo.allocators.trackers.inl>

#include <allocators/rococo.allocators.dll.inl>

DEFINE_DLL_IALLOCATOR(g_iAllocator)
DEFINE_FACTORY_DLL_IALLOCATOR_AS_BLOCK(g_iAllocator, 128, GuiRetainedModule)

#include <allocators/rococo.allocators.inl>

/* Uncomment for use with deep tracking allocator
*
#include <allocators/rococo.allocators.trackers.inl>

Rococo::Memory::AllocatorLogFlags GetDefaultMetrics()
{
	Rococo::Memory::AllocatorLogFlags flags;
	flags.LogDetailedMetrics = true;
	flags.LogLeaks = true;
	flags.LogOnModuleExit = true;
	return flags;
}

static Rococo::Memory::AllocatorLogFlags moduleLogFlags = GetDefaultMetrics();
*/

Rococo::Memory::AllocatorLogFlags GetDefaultMetrics()
{
	Rococo::Memory::AllocatorLogFlags flags;
	flags.LogDetailedMetrics = true;
	flags.LogLeaks = true;
	flags.LogOnModuleExit = true;
	return flags;
}

static Rococo::Memory::AllocatorLogFlags moduleLogFlags = GetDefaultMetrics();

DeclareAllocator(DeepTrackingAllocator, GuiRetainedModule, g_allocator)
Rococo::Memory::AllocatorMonitor<GuiRetainedModule> monitor; // When the progam terminates this object is cleared up and triggers the allocator log
OVERRIDE_MODULE_ALLOCATORS_WITH_FUNCTOR(g_allocator)