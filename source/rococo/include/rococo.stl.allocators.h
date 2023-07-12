#pragma once

#include <new> // bad_alloc, bad_array_new_length
#include <string>

#ifndef SEXYUTIL_API
# define SEXYUTIL_API ROCOCO_API_IMPORT
#endif

namespace Rococo::Memory
{
    SEXYUTIL_API void* AllocateSexyMemory(size_t nBytes);
    SEXYUTIL_API void FreeSexyMemory(void* pBuffer, size_t nBytes);
    SEXYUTIL_API void FreeSexyUnknownMemory(void* pBuffer);
    SEXYUTIL_API IAllocator& GetSexyAllocator();
    SEXYUTIL_API void SetSexyAllocator(IAllocator* allocator);
    SEXYUTIL_API void ValidateNothingAllocated();

    template <class T>
    struct SexyAllocator
    {
        typedef T value_type;
        SexyAllocator() noexcept {} //default ctor not required by C++ Standard Library

        // A converting copy constructor:
        template<class U> SexyAllocator(const SexyAllocator<U>&) noexcept {}
        template<class U> bool operator==(const SexyAllocator<U>&) const noexcept
        {
            return true;
        }
        template<class U> bool operator!=(const SexyAllocator<U>&) const noexcept
        {
            return false;
        }
        T* allocate(const size_t n) const;
        void deallocate(T* const p, size_t) const noexcept;
    };

    template <class T>
    T* SexyAllocator<T>::allocate(const size_t n) const
    {
        if (n == 0)
        {
            return nullptr;
        }
        if (n > static_cast<size_t>(-1) / sizeof(T))
        {
            throw std::bad_array_new_length();
        }
        void* const pv = AllocateSexyMemory(n * sizeof(T));
        if (!pv) { throw std::bad_alloc(); }
        return static_cast<T*>(pv);
    }

    template<class T>
    void SexyAllocator<T>::deallocate(T* const p, size_t len) const noexcept
    {
        FreeSexyMemory(p, len);
    }
}

namespace Rococo
{
    using rstdstring = std::basic_string<char, std::char_traits<char>, Memory::SexyAllocator<char>>;

    namespace Memory
    {
        // This will give some debugging info for outstanding allocations - providing the sexy default allocator is active
        // RecordAllocations(size_t) can be used to report more detailed info on outstanding allocations of a particular size
        SEXYUTIL_API void MarkMemory(cstr msg, cstr function, int line);

        // This will record callstacks for objects allocated with a given size - providing the sexy default allocator is active
        SEXYUTIL_API void RecordAllocations(size_t nBytes);
    }
}

#define MARK_MEMORY(msg) Rococo::Memory::MarkMemory(msg, __func__, __LINE__);