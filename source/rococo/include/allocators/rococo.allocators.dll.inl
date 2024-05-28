#pragma once

#include <rococo.allocators.h>

#define DEFINE_DLL_IALLOCATOR(IALLOCATORSUPERVISOR)     \
using namespace Rococo;                                 \
using namespace Rococo::Memory;                         \
IAllocatorSupervisor* IALLOCATORSUPERVISOR = nullptr;   \
void Dll_CreateAllocator();                             \
                                                        \
void* AllocateThroughDLL(size_t nBytes)                 \
{                                                       \
    if (IALLOCATORSUPERVISOR != nullptr)                \
    {                                                   \
        return IALLOCATORSUPERVISOR->Allocate(nBytes);  \
    }                                                   \
                                                        \
    OS::TripDebugger();                                 \
    throw std::bad_alloc();                             \
}                                                       \
                                                        \
void FreeThroughDLL(void* buffer)                       \
{                                                       \
    if (IALLOCATORSUPERVISOR != nullptr)                \
    {                                                   \
        IALLOCATORSUPERVISOR->FreeData(buffer);         \
        return;                                         \
    }                                                   \
                                                        \
    OS::TripDebugger();                                 \
}                                                       \
                                                        \
void FreeDLLAllocator()                                 \
{                                                       \
    if (IALLOCATORSUPERVISOR)                           \
    {                                                   \
        IALLOCATORSUPERVISOR->Free();                   \
        IALLOCATORSUPERVISOR = nullptr;                 \
    }                                                   \
}                                                       \
                                                        \
void* FirstTimeAllocator(size_t nBytes);                \
void FirstTimeDeallocator(void* buffer);                \
                                                        \
static AllocatorFunctions moduleAllocatorFunctions =    \
{                                                       \
    FirstTimeAllocator, FirstTimeDeallocator            \
};                                                      \
                                                        \
void* FirstTimeAllocator(size_t nBytes)                                 \
{                                                                       \
    if (!IALLOCATORSUPERVISOR)                                          \
    {                                                                   \
        Dll_CreateAllocator();                                          \
    }                                                                   \
                                                                        \
    moduleAllocatorFunctions = { AllocateThroughDLL , FreeThroughDLL }; \
    return AllocateThroughDLL(nBytes);                                  \
}                                                                       \
                                                                        \
void FirstTimeDeallocator(void* buffer)                                 \
{                                                                       \
    UNUSED(buffer);                                                     \
}

#define DEFINE_FACTORY_DLL_IALLOCATOR_AS_BLOCK(IALLOCATORSUPERVISOR, initialSizeInKB, friendlyName)     \
void Dll_CreateAllocator()                                                                              \
{                                                                                                       \
    IALLOCATORSUPERVISOR = Rococo::Memory::CreateBlockAllocator(initialSizeInKB, 0, #friendlyName);     \
    atexit(FreeDLLAllocator);                                                                           \
}
