#include <rococo.audio.h>
#include <rococo.allocators.h>
#include <rococo.allocators.inl>

using namespace Rococo;
using namespace Rococo::Memory;

DeclareAllocator(TrackingAllocator, AudioModule, g_allocator)  // The MallocAllocator is so far the fastest Sexy Allocator, improving release mode sexy.script.test execution by 2-3% above that of the DefaultAllocator
//DeclareAllocator(DefaultAllocator, SexyScript, g_allocator) // DefaultAllocator uses the same memory allocator at the MallocAllocator, but it adds in some metrics reported when the allocator monitor destructs
//DeclareAllocator(ScriptTrackingAllocator, SexyScript, g_allocator) // The ScriptTrackingAllocator is the slowest allocator, and requires tweaking, but allows you to get a stack trace of problematic allocations.

namespace Rococo::Audio
{
	void* AudioAllocWithNoThrow(size_t nBytes)
	{
		return operator new(nBytes, ::std::nothrow_t());
	}

	void AudioFreeMemory(void* buffer)
	{
		return operator delete(buffer);
	}

	void* AudioAlignedAlloc(size_t nBytes, int32 alignment)
	{
		return Rococo::Memory::AlignedAlloc(nBytes, alignment, AudioAllocWithNoThrow);
	}

	void AudioAlignedFree(void* buffer)
	{
		Rococo::Memory::AlignedFree(buffer, AudioFreeMemory);
	}
}

Rococo::Memory::AllocatorMonitor<AudioModule> monitor; // When the progam terminates this object is cleared up and triggers the allocator log

OVERRIDE_MODULE_ALLOCATORS_WITH_FUNCTOR(g_allocator)