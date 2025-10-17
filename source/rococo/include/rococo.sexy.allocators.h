// Copyright (c)2025 Mark Anthony Taylor. Email: mark.anthony.taylor@gmail.com. All rights reserved.
#pragma once

#include <rococo.compiler.options.h>
#include <new> // bad_alloc, bad_array_new_length
#include <sexy.util.exports.h>


#define DEFINE_SEXY_ALLOCATORS_FOR_CLASS                    \
void* operator new(size_t nBytes)                           \
{                                                           \
    return Rococo::Memory::AllocateSexyMemory(nBytes);      \
}                                                           \
                                                            \
void operator delete(void* buffer)                          \
{                                                           \
    Rococo::Memory::FreeSexyUnknownMemory(buffer);          \
}

#define DEFINE_SEXY_ALLOCATORS_OUTSIDE_OF_CLASS(CLASS_NAME) \
void* CLASS_NAME::operator new(size_t nBytes)               \
{                                                           \
    return Rococo::Memory::AllocateSexyMemory(nBytes);      \
}                                                           \
                                                            \
void CLASS_NAME::operator delete(void* buffer)              \
{                                                           \
    Rococo::Memory::FreeSexyUnknownMemory(buffer);          \
}

#ifdef USE_STD_ALLOCATOR_FOR_SEXY
#include <memory>
namespace Rococo::Memory
{
    template<class T>
    using SexyAllocator = std::allocator<T>;
}
#else
namespace Rococo::Memory
{
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
#endif
