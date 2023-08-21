#ifndef ROCOCO_ALLOCATORS_H
#define ROCOCO_ALLOCATORS_H

#ifndef ROCOCO_API 
# define ROCOCO_API __declspec(dllimport)
#endif

#include <vector>

#ifndef _WIN32

# include <stdlib.h> // Posix mem functions in OSX

namespace Rococo
{
   namespace Memory
   {
      inline void _aligned_free(void* pData)
      {
         free(pData);
      }

      inline void* _aligned_malloc(size_t nBytes, size_t alignmentByteCount)
      {
         void* pMem = nullptr;
         posix_memalign(&pMem, alignmentByteCount, nBytes);
         return pMem;
      }
   }
}
#endif

namespace Rococo::Memory
{
	ROCOCO_API [[nodiscard]] IAllocator& CheckedAllocator();
	ROCOCO_API [[nodiscard]] IAllocatorSupervisor* CreateBlockAllocator(size_t kilobytes, size_t maxkilobytes, const char* const name);
	ROCOCO_API [[nodiscard]] IAllocatorSupervisor* CreateTrackingAllocator(size_t kilobytes, size_t maxkilobytes, const char* const name);
	ROCOCO_API void* AlignedAlloc(size_t nBytes, int32 alignment, void* allocatorFunction(size_t));
	ROCOCO_API void AlignedFree(void* buffer, void deleteFunction(void*));

	struct AllocatorMetrics
	{
		size_t totalAllocationSize = 0;
		size_t totalAllocations = 0;
		size_t totalFrees = 0;
		size_t blankFrees = 0;
		size_t usefulFrees = 0;
	};

	ROCOCO_API void Log(const AllocatorMetrics& stats, cstr name, cstr intro, int (*FN_LOG)(cstr format, ...));

	ROCOCO_INTERFACE IFreeListAllocator
	{
		// Returns a buffer with a byte size equal to the value supplied to the fast allocator factory.
		virtual void* AllocateBuffer() = 0;

		// Zap the buffer for good
		virtual void FreeBuffer(void* buffer) = 0;

		// Allows re-use of buffer for a later date, and if available provides almost instant re-allocation via AllocateBuffer
		virtual void ReclaimBuffer(void* buffer) = 0;
	};

	ROCOCO_INTERFACE IFreeListAllocatorSupervisor : IFreeListAllocator
	{
		virtual void Free() = 0;
	};

	ROCOCO_API IFreeListAllocatorSupervisor* CreateFreeListAllocator(size_t elementSize);

	// HomogenousAllocator<T> provides constant time allocation and freeing of memory for class T
	// and should protect against rapid reallocation of such objects from causing memory fragmentation
	template<class T> class HomogenousAllocator
	{
		std::vector<T*> freeAllocs;
		std::vector<T*> activeAllocs;
	public:
		~HomogenousAllocator()
		{
			for (auto i : activeAllocs)
			{
				_aligned_free(i);
			}
		}

		T* Allocate()
		{
			if (freeAllocs.empty())
			{
				void* buffer = _aligned_malloc(sizeof(T), 16);
				auto t = (T*)buffer;
				activeAllocs.push_back(t);
				return t;
			}
			else
			{
				T* t = freeAllocs.back();
				freeAllocs.pop_back();
				return t;
			}
		}

		void Free(T* t)
		{
			if (t)
			{
				freeAllocs.push_back(t);
			}
		}

		size_t NumberOfFreeItems() const
		{
			return freeAllocs.size();
		}

		size_t NumberOfAllocations() const
		{
			return activeAllocs.size();
		}
	};

	struct AllocatorFunctions
	{
		// Allocator function, that throws std::bad_alloc on an error, else returns a buffer of the required size
		typedef void* (*FN_ALLOCATE_MEMORY)(size_t nBytes);

		// Frees memory allocated with AllocatorFunctions::Allocate
		typedef void (*FN_FREE_MEMORY)(void* buffer);

		// Allocator function, that throws std::bad_alloc on an error, else returns a buffer of the required size
		FN_ALLOCATE_MEMORY Allocate;

		// Frees memory allocated with AllocatorFunctions::Allocate
		FN_FREE_MEMORY Free;
		const char* Name;
	};

	struct AllocatorLogFlags
	{
		// If not set, nothing is logged when a module exits. Generally set when some tool does not need to worry about its memory consumption
		bool LogOnModuleExit : 1;

		// If set provides some info about what was allocated
		bool LogDetailedMetrics : 1;

		// If set enumerates the allocations that were not freed at the point of module termination
		bool LogLeaks : 1;
	};

	ROCOCO_API bool SetDefaultAllocators(AllocatorFunctions allocatorFunctions);
	ROCOCO_API AllocatorFunctions GetDefaultAllocators();
	ROCOCO_API void SetAllocatorLogFlags(AllocatorLogFlags flags);
	ROCOCO_API AllocatorLogFlags GetAllocatorLogFlags();
}

#endif
