#include <rococo.types.h>
#include <allocators/rococo.allocators.dll.inl>

/* Uncomment for use with deep tracking allocator */

#include <allocators/rococo.allocators.trackers.inl>

DEFINE_DLL_IALLOCATOR(dx11Allocator)

Rococo::Memory::AllocatorLogFlags GetDefaultMetrics()
{
	Rococo::Memory::AllocatorLogFlags flags;
	flags.LogDetailedMetrics = true;
	flags.LogLeaks = true;
	flags.LogOnModuleExit = true;
	return flags;
}

static Rococo::Memory::AllocatorLogFlags moduleLogFlags = GetDefaultMetrics();

DEFINE_FACTORY_DLL_IALLOCATOR_AS_BLOCK(dx11Allocator, 128, DX11Module)

#define DEEP_TRACK_DX11_ALLOCATIONS 0

#if DEEP_TRACK_DX11_ALLOCATIONS
DeclareAllocator(DeepTrackingAllocator, DX11Module, g_allocator)
#else
#include <allocators/rococo.allocators.inl>
DeclareAllocator(TrackingAllocator, DX11Module, g_allocator)
#endif


Rococo::Memory::AllocatorMonitor<DX11Module> monitor; // When the progam terminates this object is cleared up and triggers the allocator log
OVERRIDE_MODULE_ALLOCATORS_WITH_FUNCTOR(g_allocator)

#define ROCOCO_DX_API ROCOCO_API_EXPORT

ROCOCO_DX_API void ReportMemoryStatus()
{
	g_allocator.Log("DX11Logs", "Memory Status Report");
}

namespace Rococo::Memory
{
	Rococo::IAllocator& GetDX11Allocator()
	{
		return *dx11Allocator;
	}
}