#include <rococo.audio.h>
#include <rococo.allocators.h>

using namespace Rococo;
using namespace Rococo::Memory;

namespace Rococo::Audio
{
	void FreeAudioAllocator();

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

#include <allocators/rococo.allocators.dll.inl>


DEFINE_DLL_IALLOCATOR(audioAllocator)
DEFINE_FACTORY_DLL_IALLOCATOR_AS_BLOCK(audioAllocator, 128, Audio)

#include <allocators/rococo.allocators.inl>

DeclareAllocator(TrackingAllocator, AudioModule, g_allocator)
Rococo::Memory::AllocatorMonitor<AudioModule> monitor; // When the progam terminates this object is cleared up and triggers the allocator log
OVERRIDE_MODULE_ALLOCATORS_WITH_FUNCTOR(g_allocator)