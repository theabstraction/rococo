#include <rococo.types.h>
#include <allocators/rococo.allocators.dll.inl>

DEFINE_DLL_IALLOCATOR(dx11Allocator)
DEFINE_FACTORY_DLL_IALLOCATOR_AS_BLOCK(dx11Allocator, 128, DX11Module)

#include <allocators/rococo.allocators.inl>

DeclareAllocator(TrackingAllocator, DX11Module, g_allocator)
Rococo::Memory::AllocatorMonitor<DX11Module> monitor; // When the progam terminates this object is cleared up and triggers the allocator log
OVERRIDE_MODULE_ALLOCATORS_WITH_FUNCTOR(g_allocator)

#define ROCOCO_DX_API ROCOCO_API_EXPORT

ROCOCO_DX_API void ReportMemoryStatus()
{
	g_allocator.Log("DX11Logs", "Memory Status Report");
}