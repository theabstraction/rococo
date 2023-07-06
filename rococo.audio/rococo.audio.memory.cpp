#include <rococo.api.h>
#include <memory.h>
#include <new>

using namespace Rococo;

namespace
{
	static Rococo::IAllocator* s_AudioAllocator = nullptr;
	static size_t allocCount = 0;
	static AutoFree<Rococo::IAllocatorSupervisor> s_privateHeap;
}

namespace Rococo::Audio
{
	ROCOCO_AUDIO_API void SetAudioAllocator(IAllocator* allocator)
	{
		if (allocCount > 0)
		{
			Throw(0, "%s: The audio system has already outstanding allocations.\n The API consumer needs to free all audio memory before assigning a new allocator.", __FUNCTION__);
		}

		s_AudioAllocator = allocator;
	}

	void* AudioAllocWithThrow(size_t nBytes)
	{
		void* pBuffer;

		if (s_AudioAllocator)
		{
			pBuffer = s_AudioAllocator->Allocate(nBytes);
		}
		else
		{
			pBuffer = _aligned_malloc(nBytes, 16);
		}

		if (!pBuffer)
		{
			Rococo::Throw(0, "Could not reserve %llu bytes with the %s audio allocator for an audio resource", nBytes, s_AudioAllocator ? "assigned" : "_aligned_malloc");
		}
		
		allocCount++;
		return pBuffer;
	}

	void* AudioAllocWithNoThrow(size_t nBytes)
	{
		try
		{
			auto* buf = AudioAllocWithThrow(nBytes);
			return buf;
		}
		catch (...)
		{
			return nullptr;
		}
	}

	void AudioFreeMemory(void* buffer)
	{
		if (!buffer) return;

		if (s_AudioAllocator)
		{
			s_AudioAllocator->FreeData(buffer);
		}
		else
		{
			_aligned_free(buffer);
		}

		allocCount--;
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

#ifdef _WIN32
_NODISCARD _Ret_notnull_ _Post_writable_byte_size_(nBytes) _VCRT_ALLOCATOR
void* __CRTDECL operator new(std::size_t nBytes)
{
	return Rococo::Audio::AudioAllocWithThrow(nBytes);
}

_Ret_maybenull_ _Success_(return != NULL) _Post_writable_byte_size_(nBytes) _VCRT_ALLOCATOR
void* __CRTDECL operator new(size_t nBytes, ::std::nothrow_t const&) noexcept
{
	return Rococo::Audio::AudioAllocWithNoThrow(nBytes);
}

_NODISCARD _Ret_notnull_ _Post_writable_byte_size_(nBytes) _VCRT_ALLOCATOR
void* __CRTDECL operator new[](size_t nBytes)
{
	return Rococo::Audio::AudioAllocWithThrow(nBytes);
}

_NODISCARD _Ret_maybenull_ _Success_(return != NULL) _Post_writable_byte_size_(nBytes) _VCRT_ALLOCATOR
void* __CRTDECL operator new[](size_t nBytes, ::std::nothrow_t const&) noexcept
{
	return Rococo::Audio::AudioAllocWithNoThrow(nBytes);
}

void operator delete(void* buffer) throw()
{
	return Rococo::Audio::AudioFreeMemory(buffer);
}

#endif