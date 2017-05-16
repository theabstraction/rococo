#include <rococo.types.h>
#include <stdlib.h>
#include <rococo.strings.h>

#ifdef _WIN32
# include <rococo.os.win32.h>
#else

namespace
{
      using namespace Rococo;
      void OutputDebugStringA(cstr text)
      {
         puts(text);
      }

      typedef void* HANDLE;

      HANDLE HeapCreate(int unused, size_t start, size_t capacity)
      {
         return nullptr;
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

   class CheckedAllocator: public IAllocator
   {
      uint32 allocCount{ 0 };
      uint32 freeCount{ 0 };
      uint32 reallocCount{ 0 };
   public:
      ~CheckedAllocator()
      {
         rchar text[1024];
         SafeFormat(text, _TRUNCATE, "\nCheckedAllocator: Allocs: %u, Frees: %u, Reallocs: %u\n\n", allocCount, freeCount, reallocCount);
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
   public:
      BlockAllocator(size_t kilobytes, size_t maxkilobytes)
      {
         hHeap = HeapCreate(HEAP_NO_SERIALIZE, kilobytes * 1024, maxkilobytes * 1024);
         if (hHeap == nullptr) Throw(GetLastError(), "Error allocating heap");
      }

      ~BlockAllocator()
      {
         rchar text[1024];
         SafeFormat(text, _TRUNCATE, "\nBlockAllocator(%p) Allocs: %u, Frees: %u, Reallocs: %u\n\n", hHeap, allocCount, freeCount, reallocCount);
         OutputDebugStringA(text);

         HeapDestroy(hHeap);
      }

      virtual void* Allocate(size_t capacity)
      {
         allocCount++;
         auto* ptr = HeapAlloc(hHeap, 0, capacity);
         if (ptr == nullptr) Throw(0, "Insufficient memory in dedicated BlockAllocator heap for alloc operation");
         return ptr;
      }

      virtual void FreeData(void* data)
      {
         freeCount++;
         if (data) HeapFree(hHeap, 0, data);
      }

      virtual void* Reallocate(void* old, size_t capacity)
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

      virtual void Free()
      {
         delete this;
      }
   };
}

namespace Rococo
{
   namespace Memory
   {
      IAllocator& CheckedAllocator() 
      {
         return s_CheckedAllocator;
      }

      IAllocatorSupervisor* CreateBlockAllocator(size_t kilobytes, size_t maxkilobytes)
      {
         return new BlockAllocator(kilobytes, maxkilobytes);
      }
   }
}