#define ROCOCO_API __declspec(dllexport)

#include <rococo.types.h>
#include <rococo.allocators.h>

#include <rococo.strings.h>
#include <stdlib.h>
#include <vector>

#ifdef _WIN32
# include <rococo.os.win32.h>
#else
# include <stdio.h>

namespace
{
      using namespace Rococo;
      void OutputDebugStringA(cstr text)
      {
         printf("%s\n", text);
      }

      typedef void* HANDLE;

      HANDLE HeapCreate(int unused, size_t start, size_t capacity)
      {
         return (void*) 1;
      }

      int GetLastError()
      {
         return 0;
      }

      void HeapDestroy(HANDLE hHeap)
      {

      }

      void* HeapAlloc(HANDLE hHeap, int unused, size_t capacity)
      {
         return malloc(capacity);
      }

      void HeapFree(HANDLE hHeap, int unused, void* data)
      {
         free(data);
      }

      void* HeapReAlloc(HANDLE hHeap, int unused, void* old, size_t capacity)
      {
         return realloc(old, capacity);
      }

      enum {  HEAP_NO_SERIALIZE = 0 };
} // anon
#endif

#include <vector>
#include <algorithm>
#include <rococo.allocator.template.h>

namespace
{
   using namespace Rococo;
   using namespace Rococo::Strings;

   class CheckedAllocator: public IAllocator
   {
      uint32 allocCount{ 0 };
      uint32 freeCount{ 0 };
      uint32 reallocCount{ 0 };

      std::vector<FN_AllocatorReleaseFunction, AllocatorWithMalloc<FN_AllocatorReleaseFunction>> atReleaseQueue;

   public:
      ~CheckedAllocator()
      {
         char text[1024];
         SafeFormat(text, sizeof(text), "\nCheckedAllocator: Allocs: %u, Frees: %u, Reallocs: %u\n\n", allocCount, freeCount, reallocCount);
         OutputDebugStringA(text);

         for (auto fn : atReleaseQueue)
         {
             fn();
         }
      }

      void* Allocate(size_t capacity) override
      {
         allocCount++;
         return malloc(capacity);
      }

      void FreeData(void* data) override
      {
         freeCount++;
         free(data);
      }

      void* Reallocate(void* ptr, size_t capacity) override
      {
         if (ptr == nullptr)
         {
            return Allocate(capacity);
         }
         reallocCount++;
         return realloc(ptr, capacity);
      }

      void AtRelease(FN_AllocatorReleaseFunction fn) override
      {
          auto i = std::find(atReleaseQueue.begin(), atReleaseQueue.end(), fn);
          if (i != atReleaseQueue.end())
          {
              atReleaseQueue.push_back(fn);
          }
      }

      size_t EvaluateHeapSize()
      {
          return 0;
      }
   } s_CheckedAllocator;

   class BlockAllocator : public IAllocatorSupervisor
   {
      HANDLE hHeap{ nullptr };
      uint32 allocCount{ 0 };
      uint32 freeCount{ 0 };
      uint32 reallocCount{ 0 };
	  size_t maxBytes;
      const char* const name;

      std::vector<FN_AllocatorReleaseFunction, AllocatorWithMalloc<FN_AllocatorReleaseFunction>> atReleaseQueue;
   public:
      BlockAllocator(size_t kilobytes, size_t _maxkilobytes, const char* const _name): maxBytes(_maxkilobytes * 1024), name(_name)
      {
         hHeap = HeapCreate(0, kilobytes * 1024, maxBytes);
         if (hHeap == nullptr) Throw(GetLastError(), "Error allocating heap");
      }

      ~BlockAllocator()
      {
          for (auto fn : atReleaseQueue)
          {
              fn();
          }
         
          size_t totalAllocation = EvaluateHeapSize();

          char text[1024];
          SafeFormat(text, sizeof(text), "\nBlockAllocator(%s) Allocs: %u, Frees: %u, Reallocs: %u. Heap size: %llu MB\n", name, allocCount, freeCount, reallocCount, totalAllocation / 1_megabytes);
          OutputDebugStringA(text);

          if (allocCount > freeCount)
          {
              SafeFormat(text, sizeof(text), "Memory leaked. %llu allocations were not freed\n\n", allocCount - freeCount);
              OutputDebugStringA(text);
          }

          HeapDestroy(hHeap);
      }

	  void* Allocate(size_t capacity) override
	  {
		  if (capacity > 0x7FFF8 && maxBytes != 0)
		  {
			  Throw(GetLastError(), "Heap max must be set to zero (growable heap for allocations this large) %llu", maxBytes);
		  }
		  allocCount++;
		  auto* ptr = HeapAlloc(hHeap, 0, capacity);
		  if (ptr == nullptr) Throw(GetLastError(), "Insufficient memory in dedicated BlockAllocator heap for alloc operation. Heap max size is %llu bytes", maxBytes);
		  return ptr;
	  }

      void AtRelease(FN_AllocatorReleaseFunction fn) override
      {
          auto i = std::find(atReleaseQueue.begin(), atReleaseQueue.end(), fn);
          if (i == atReleaseQueue.end())
          {
              atReleaseQueue.push_back(fn);
          }
      }

      void FreeData(void* data) override
      {
          if (data)
          {
              freeCount++;
              if (data) HeapFree(hHeap, 0, data);
          }
      }

      void* Reallocate(void* old, size_t capacity) override
      {
         if (old == nullptr)
         {
            return Allocate(capacity);
         }
         reallocCount++;
         auto* ptr = HeapReAlloc(hHeap, 0, old, capacity);
         if (ptr == nullptr) Throw(0, "Insufficient memory in dedicated BlockAllocator heap for realloc operation");
         return ptr;
      }

      void Free() override
      {
         delete this;
      }

      size_t EvaluateHeapSize() override
      {
          PROCESS_HEAP_ENTRY entry;
          entry = { 0 };

          size_t totalAllocation = 0;
          while (HeapWalk(hHeap, &entry))
          {
              totalAllocation += (entry.cbData + entry.cbOverhead);
          }

          return totalAllocation;
      }
   };
}

namespace  Rococo::Memory::ANON
{
    struct FreeListAllocator :IFreeListAllocatorSupervisor
    {
        size_t elementSize;

        std::vector<void*> freeList;

        FreeListAllocator(size_t varElementSize) : elementSize(varElementSize)
        {

        }

        virtual ~FreeListAllocator()
        {
            for (auto* v : freeList)
            {
                FreeBuffer(v);
            }
        }

        void* AllocateBuffer() override
        {
            if (freeList.empty())
            {
                return new char[elementSize];
            }
            else
            {
                void* lastElement = freeList.back();
                freeList.pop_back();
                return lastElement;
            }
        }

        void FreeBuffer(void* buffer) override
        {
            delete[] buffer;
        }

        void ReclaimBuffer(void* buffer) override
        {
            if (buffer == nullptr) return;
            freeList.push_back(buffer);
        }

        void Free() override
        {
            delete this;
        }
    };
}

namespace Rococo::Memory
{
    ROCOCO_API IFreeListAllocatorSupervisor* CreateFreeListAllocator(size_t elementSize)
    {
        return new ANON::FreeListAllocator(elementSize);
    }

    ROCOCO_API IAllocator& CheckedAllocator()
    {
        return s_CheckedAllocator;
    }

    ROCOCO_API IAllocatorSupervisor* CreateBlockAllocator(size_t kilobytes, size_t maxkilobytes, const char* const name)
    {
        return new BlockAllocator(kilobytes, maxkilobytes, name);
    }

    ROCOCO_API void* AlignedAlloc(size_t nBytes, int32 alignment, void* allocatorFunction(size_t))
    {
        void* p1; // original block
        void** p2; // aligned block
        int offset = alignment - 1 + sizeof(void*);
        if ((p1 = (void*)allocatorFunction(nBytes + offset)) == NULL)
        {
            return nullptr;
        }
        p2 = (void**)(((size_t)(p1)+offset) & ~(alignment - 1));
        p2[-1] = p1;
        return p2;
    };

    ROCOCO_API void AlignedFree(void* buffer, void deleteFunction(void*))
    {
        if (buffer) deleteFunction(((void**)buffer)[-1]);
    };

    ROCOCO_API void Log(const AllocatorMetrics& stats, cstr name, cstr intro, int (*FN_LOG)(cstr format, ...))
    {
        FN_LOG("%s:  %s\n", name, intro);
        FN_LOG(" Total allocation size: %llu bytes\n", stats.totalAllocationSize);
        FN_LOG(" Total allocations: %llu\n", stats.totalAllocations);
        FN_LOG(" Useful frees: %llu\n", stats.usefulFrees);
        FN_LOG(" Total frees: %llu\n", stats.totalFrees);
        FN_LOG(" Blank frees: %llu\n\n", stats.blankFrees);
    }

    AllocatorFunctions defaultAllocatorFunctions{ 0 };

    void* DefaultMallocWithBadAllocException(size_t nBytes)
    {
        void* buffer = malloc(nBytes);
        if (buffer != nullptr)
        {
            return buffer;
        }
        
        throw std::bad_alloc();
    }

    ROCOCO_API bool SetDefaultAllocators(AllocatorFunctions allocatorFunctions)
    {
        if (defaultAllocatorFunctions.Allocate != nullptr)
        {
            return false; // Allocator have already been initialized
        }

        defaultAllocatorFunctions = allocatorFunctions;
        return true;
    }

    ROCOCO_API AllocatorFunctions GetDefaultAllocators()
    {
        if (defaultAllocatorFunctions.Allocate == nullptr)
        {
            defaultAllocatorFunctions.Allocate = DefaultMallocWithBadAllocException;
            defaultAllocatorFunctions.Free = free;
            defaultAllocatorFunctions.Name = "Rococo::Utils defaults(malloc++/free)";
        }

        return defaultAllocatorFunctions;
    }

    AllocatorLogFlags logFlags = { 0 };

    ROCOCO_API void SetAllocatorLogFlags(AllocatorLogFlags flags)
    {
        logFlags = flags;
    }

    ROCOCO_API AllocatorLogFlags GetAllocatorLogFlags()
    {
        return logFlags;
    }
} // Rococo::Memory