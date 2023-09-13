#include "sexy.script.stdafx.h"
#include <rococo.sexy.map.expert.h>

using namespace Rococo::Memory;

//#define ADD_LEAK_TRACKING
// #define ROCOCO_MEMORY_MANAGEMENT // If defined we use the latest RococoUtils allocators for memory management, otherwise fallback on ISexyAllocator based management for global new and delete operators

#if IS_SCRIPT_DLL
# ifdef ROCOCO_MEMORY_MANAGEMENT
#  define SEXY_SCRIPT_ALLOCATOR_IS_VIA_SEXY_ALLOCATOR false
# else
#  define SEXY_SCRIPT_ALLOCATOR_IS_VIA_SEXY_ALLOCATOR true
# endif 

# if !SEXY_SCRIPT_ALLOCATOR_IS_VIA_SEXY_ALLOCATOR

#  ifdef _DEBUG
#    ifdef ADD_LEAK_TRACKING
#     include <rococo.allocators.trackers.inl>
	  DeclareAllocator(DeepTrackingAllocator, SexyScript, g_allocator) 
#    else
#     include <rococo.allocators.inl>
      DeclareAllocator(FastAllocator, SexyScript, g_allocator)
#    endif
#  else // release mode
#   include <rococo.allocators.inl>
    DeclareAllocator(FastAllocator, SexyScript, g_allocator)
#  endif

 Rococo::Memory::AllocatorMonitor<SexyScript> monitor; // When the progam terminates this object is cleared up and triggers the allocator log
 OVERRIDE_MODULE_ALLOCATORS_WITH_FUNCTOR(g_allocator)

// else - Route allocations through the SexyAllocator interface
# else

// If defined will use std allocators rather than those for specific for sexy. Best used/defined when SexyScript is shipped in DLL modules
#ifdef USE_STD_ALLOCATOR_FOR_SEXY 
# error "Undefine USE_STD_ALLOCATOR_FOR_SEXY -> otherwise this code results in infintie recrusion"
#endif

_NODISCARD _Ret_notnull_ _Post_writable_byte_size_(nBytes) _VCRT_ALLOCATOR
void* __CRTDECL operator new(std::size_t nBytes)
{
	return Rococo::Memory::AllocateSexyMemory(nBytes);
}

_Ret_maybenull_ _Success_(return != NULL) _Post_writable_byte_size_(nBytes) _VCRT_ALLOCATOR
void* __CRTDECL operator new(size_t nBytes, ::std::nothrow_t const&) noexcept
{
	try
	{
		return Rococo::Memory::AllocateSexyMemory(nBytes);
	}
	catch (...)
	{
		return nullptr;
	}
}

_NODISCARD _Ret_notnull_ _Post_writable_byte_size_(nBytes) _VCRT_ALLOCATOR
void* __CRTDECL operator new[](size_t nBytes)
{
	return Rococo::Memory::AllocateSexyMemory(nBytes);
}

_NODISCARD _Ret_maybenull_ _Success_(return != NULL) _Post_writable_byte_size_(nBytes) _VCRT_ALLOCATOR
void* __CRTDECL operator new[](size_t nBytes, ::std::nothrow_t const&) noexcept
{
	try
	{
		return Rococo::Memory::AllocateSexyMemory(nBytes);
	}
	catch (...)
	{
		return nullptr;
	}
}

void operator delete(void* buffer) throw()
{
	Rococo::Memory::FreeSexyUnknownMemory(buffer);
}
# endif // SEXY_SCRIPT_MANAGEMENT_IS_VIA_SEXY_ALLOCATOR
#else // IS_SCRIPT_DLL = 0
// No specific module allocation, rely on the statically linked DLL or EXE to global new and delete operations
#endif // IS_SCRIPT_DLL = 0

namespace Rococo::Script
{
	DEFINE_SEXY_ALLOCATORS_OUTSIDE_OF_CLASS(ArrayImage);
	DEFINE_SEXY_ALLOCATORS_OUTSIDE_OF_CLASS(CStringConstant);
	DEFINE_SEXY_ALLOCATORS_OUTSIDE_OF_CLASS(RawReflectionBinding);
}