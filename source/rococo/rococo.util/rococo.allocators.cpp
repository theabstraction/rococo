// Copyright (c)2025 Mark Anthony Taylor. Email: mark.anthony.taylor@gmail.com. All rights reserved.
#include <rococo.types.h>
#include <rococo.allocators.h>
#include <rococo.os.h>
#include <rococo.debugging.h>

#include <rococo.strings.h>
#include <stdlib.h>
#include <vector>
#include <unordered_map>

#include <rococo.debugging.h>

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
#include <allocators/rococo.allocator.malloc.h>

using namespace Rococo;
using namespace Rococo::Strings;
using namespace Rococo::Debugging;

#ifdef _WIN32

namespace MSWindowsHeap
{
    using namespace MSWindows;

    struct PROCESS_HEAP_ENTRY
    {
        PROCESS_HEAP_ENTRY() {}

        void* lpData;
        DWORD cbData;
        BYTE cbOverhead;
        BYTE iRegionIndex;
        WORD wFlags;
        union Items
        {
            struct Block
            {
                HANDLE hMem;
                DWORD dwReserved[3];
            } block;
            struct Region
            {
                DWORD dwCommittedSize;
                DWORD dwUnCommittedSize;
                LPVOID lpFirstBlock;
                LPVOID lpLastBlock;
            } region;
        } items;
    };

    static_assert(sizeof(PROCESS_HEAP_ENTRY) == 40);

    typedef PROCESS_HEAP_ENTRY* LPPROCESS_HEAP_ENTRY;
    typedef PROCESS_HEAP_ENTRY* PPROCESS_HEAP_ENTRY;


    extern "C" __declspec(dllimport) HANDLE HeapCreate(DWORD flOptions, SIZE_T dwInitialSize, SIZE_T dwMaximumSize);
    extern "C" __declspec(dllimport) BOOL HeapDestroy(HANDLE hHeap);
    extern "C" __declspec(dllimport) LPVOID HeapAlloc(HANDLE hHeap, DWORD dwFlags, SIZE_T dwBytes);
    extern "C" __declspec(dllimport) LPVOID HeapReAlloc(HANDLE hHeap, DWORD dwFlags, LPVOID lpMem, SIZE_T dwBytes);
    extern "C" __declspec(dllimport) BOOL HeapFree(HANDLE hHeap, DWORD dwFlags, LPVOID lpMem);
    extern "C" __declspec(dllimport) SIZE_T HeapSize(HANDLE hHeap, DWORD dwFlags, LPCVOID lpMem);

    extern "C" __declspec(dllimport) BOOL HeapWalk(HANDLE hHeap, LPPROCESS_HEAP_ENTRY lpEntry);
}

using namespace MSWindowsHeap;
#endif

namespace Rococo::Memory::ANON
{
    class CheckedAllocator : public IAllocator
    {
        uint32 allocCount{ 0 };
        uint32 freeCount{ 0 };
        uint32 reallocCount{ 0 };

        std::vector<FN_AllocatorReleaseFunction, AllocatorWithMalloc<FN_AllocatorReleaseFunction>> atReleaseQueue;

    public:
        virtual ~CheckedAllocator()
        {
            for (auto fn : atReleaseQueue)
            {
                fn();
            }

            PrintD("\nCheckedAllocator: Allocs: %u, Frees: %u, Reallocs: %u\n\n", allocCount, freeCount, reallocCount);
        }

        void* Allocate(size_t capacity) override
        {
            allocCount++;
            void* buf = malloc(capacity);
            if (buf == nullptr)
            {
                throw std::bad_alloc();
            }

            return buf;
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


#ifdef _WIN32
    class BlockAllocator : public IAllocatorSupervisor
    {
        HANDLE hHeap;
        uint32 allocCount{ 0 };
        uint32 freeCount{ 0 };
        uint32 reallocCount{ 0 };
        size_t maxBytes;
        const char* const name;

        std::vector<FN_AllocatorReleaseFunction, AllocatorWithMalloc<FN_AllocatorReleaseFunction>> atReleaseQueue;
    public:
        BlockAllocator(size_t kilobytes, size_t _maxkilobytes, const char* const _name) : maxBytes(_maxkilobytes * 1024), name(_name)
        {
            hHeap = HeapCreate(0, kilobytes * 1024, maxBytes);
            if (IsNull(hHeap)) Throw(GetLastError(), "Error allocating heap");
        }

        virtual ~BlockAllocator()
        {
            for (auto fn : atReleaseQueue)
            {
                fn();
            }

            size_t totalAllocation = EvaluateHeapSize();

            PrintD("\nBlockAllocator(%s) Allocs: %u, Frees: %u, Reallocs: %u. Heap size: %llu MB\n", name, allocCount, freeCount, reallocCount, totalAllocation / 1_megabytes);

            if (allocCount > freeCount)
            {
                PrintD("Memory leaked. %llu allocations were not freed\n\n", allocCount - freeCount);
            }

            HeapDestroy(hHeap);
        }

        void* Allocate(size_t capacity) override
        {
            if (capacity > 0x7FFF8 && maxBytes != 0)
            {
                char msg[256];
                SafeFormat(msg, "Heap max must be set to zero (growable heap for allocations this large) %llu", maxBytes);
                Rococo::Debugging::AddCriticalLog(msg);
                throw std::bad_alloc();
            }
            allocCount++;
            auto* ptr = HeapAlloc(hHeap, 0, capacity);
            if (ptr == nullptr) throw std::bad_alloc();
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
            memset(&entry, 0, sizeof entry);

            size_t totalAllocation = 0;
            while (HeapWalk(hHeap, &entry))
            {
                totalAllocation += (entry.cbData + entry.cbOverhead);
            }

            return totalAllocation;
        }
    };
#else
    class BlockAllocator : public IAllocatorSupervisor
    {
        const char* const name;

        std::vector<FN_AllocatorReleaseFunction, AllocatorWithMalloc<FN_AllocatorReleaseFunction>> atReleaseQueue;
    public:
        BlockAllocator(size_t /* kilobytes - unused */, size_t /* maxBytes - unused */, const char* const _name) : name(_name)
        {
        }

        virtual ~BlockAllocator()
        {
            for (auto fn : atReleaseQueue)
            {
                fn();
            }
        }

        void* Allocate(size_t capacity) override
        {
            return malloc(capacity);
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
            free(data);
        }

        void* Reallocate(void* old, size_t capacity) override
        {
            free(old);
            return malloc(capacity);
        }

        void Free() override
        {
            delete this;
        }

        size_t EvaluateHeapSize() override
        {
            return 0;
        }
    };
#endif

    struct TrackingData
    {
        size_t capacity;
        StackFrame::Address addr;
    };

    class TrackingAllocator : public IAllocatorSupervisor
    {
        cstr name;
        std::vector<FN_AllocatorReleaseFunction, AllocatorWithMalloc<FN_AllocatorReleaseFunction>> atReleaseQueue;
        std::unordered_map<void*, TrackingData> mapAllocToData;

        enum { TRACK_SIZE = 47, TRACK_DEPTH = 6 };
    public:
        TrackingAllocator(cstr _name) :name(_name)
        {
        }

        virtual ~TrackingAllocator()
        {
            for (auto fn : atReleaseQueue)
            {
                fn();
            }

            if (mapAllocToData.empty())
            {
                return;
            }

            PrintD("\nTracking Allocator(%s): Leaks detected: %llu buffers\n", name, mapAllocToData.size());

            int count = 0;
            for (auto& i : mapAllocToData)
            {
                PrintD("Leak #%d: %llu bytes\n", ++count, i.second.capacity);
            }

            if constexpr (TRACK_SIZE != 0)
            {
                for (auto& i : mapAllocToData)
                {
                    auto addr = i.second.addr;

                    char buffer[1024];
                    FormatStackFrame(buffer, sizeof(buffer), addr);

                    PrintD("Leak allocated at %s\n", buffer);
                }
            }
        }

        void* Allocate(size_t capacity) override
        {
            auto* buffer = malloc(capacity);

            if (!buffer)
            {
                throw std::bad_alloc();
            }

            if constexpr(TRACK_SIZE == 0)
            {

                TrackingData data{ capacity, 0 };
                mapAllocToData.insert(std::make_pair(buffer, data));
            }
            else if (capacity == TRACK_SIZE)
            {
                TrackingData data{ capacity, FormatStackFrame(nullptr, 0, TRACK_DEPTH) };
                mapAllocToData.insert(std::make_pair(buffer, data));
            }

            return buffer;
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
                mapAllocToData.erase(data);
            }

            free(data);
        }

        void* Reallocate(void* old, size_t capacity) override
        {
            UNUSED(old);
            return Allocate(capacity);
        }

        void Free() override
        {
            delete this;
        }

        size_t EvaluateHeapSize()
        {
            return 0;
        }
    };

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
            char* cBuffer = (char*)buffer;
            delete[] cBuffer;
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
        return ANON::s_CheckedAllocator;
    }

    ROCOCO_API IAllocatorSupervisor* CreateTrackingAllocator(size_t kilobytes, size_t maxkilobytes, const char* const name)
    {
        UNUSED(kilobytes);
        UNUSED(maxkilobytes);
        return new ANON::TrackingAllocator(name);
    }

    ROCOCO_API IAllocatorSupervisor* CreateBlockAllocator(size_t kilobytes, size_t maxkilobytes, const char* const name)
    {
        return new ANON::BlockAllocator(kilobytes, maxkilobytes, name);
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
        FN_LOG(" Total allocation size: %llu KB\n", stats.totalAllocationSize / 1024);
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

    ROCOCO_API void* rococo_aligned_malloc(size_t alignment, size_t bufferLength)
    {
        void* raw = malloc(bufferLength + alignment - 1 + sizeof(void*));
        if (!raw) return NULL;

        void** aligned = (void**)(((uintptr_t)raw + sizeof(void*) + alignment - 1) & ~(alignment - 1));
        aligned[-1] = raw; // Store original pointer for freeing
        return aligned;
    }

    ROCOCO_API void rococo_aligned_free(void* pData)
    {
        free(((void**)pData)[-1]);
    }
} // Rococo::Memory