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

namespace
{
   using namespace Rococo;
   using namespace Rococo::Strings;

   class CheckedAllocator: public IAllocator
   {
      uint32 allocCount{ 0 };
      uint32 freeCount{ 0 };
      uint32 reallocCount{ 0 };
   public:
      ~CheckedAllocator()
      {
         char text[1024];
         SafeFormat(text, sizeof(text), "\nCheckedAllocator: Allocs: %u, Frees: %u, Reallocs: %u\n\n", allocCount, freeCount, reallocCount);
         OutputDebugStringA(text);
      }

      virtual void* Allocate(size_t capacity)
      {
         allocCount++;
         return malloc(capacity);
      }

      virtual void FreeData(void* data)
      {
         freeCount++;
         free(data);
      }

      virtual void* Reallocate(void* ptr, size_t capacity)
      {
         if (ptr == nullptr)
         {
            return Allocate(capacity);
         }
         reallocCount++;
         return realloc(ptr, capacity);
      }
   } s_CheckedAllocator;

   class BlockAllocator : public IAllocatorSupervisor
   {
      HANDLE hHeap{ nullptr };
      uint32 allocCount{ 0 };
      uint32 freeCount{ 0 };
      uint32 reallocCount{ 0 };
	  size_t maxBytes;
   public:
      BlockAllocator(size_t kilobytes, size_t _maxkilobytes): maxBytes(_maxkilobytes * 1024)
      {
         hHeap = HeapCreate(0, kilobytes * 1024, maxBytes);
         if (hHeap == nullptr) Throw(GetLastError(), "Error allocating heap");
      }

      ~BlockAllocator()
      {
         char text[1024];
         SafeFormat(text, sizeof(text), "\nBlockAllocator(%p) Allocs: %u, Frees: %u, Reallocs: %u\n\n", hHeap, allocCount, freeCount, reallocCount);
         OutputDebugStringA(text);

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

      void FreeData(void* data) override
      {
         freeCount++;
         if (data) HeapFree(hHeap, 0, data);
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
   };
}

namespace
{
    using namespace Rococo::Memory;

    struct FreeListAllocator :IFreeListAllocatorSupervisor
    {
        size_t elementSize;

        std::vector<void*> freeList;

        FreeListAllocator(size_t varElementSize) : elementSize(varElementSize)
        {

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
        return new FreeListAllocator(elementSize);
    }

    ROCOCO_API IAllocator& CheckedAllocator()
    {
        return s_CheckedAllocator;
    }

    ROCOCO_API IAllocatorSupervisor* CreateBlockAllocator(size_t kilobytes, size_t maxkilobytes)
    {
        return new BlockAllocator(kilobytes, maxkilobytes);
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
}