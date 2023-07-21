#include <rococo.allocators.h>

#define USE_ROCOCO_UTILS_FOR_MODULE_ALLOCATORS												\
static void* FirstTimeAllocator(size_t nBytes);												\
static void FirstTimeDeallocator(void* buffer);												\
																							\
static Rococo::Memory::AllocatorFunctions moduleAllocatorFunctions = 						\
{																							\
	FirstTimeAllocator, 																	\
	FirstTimeDeallocator 																	\
};																							\
																							\
static Rococo::Memory::AllocatorLogFlags moduleLogFlags;									\
																							\
static void* FirstTimeAllocator(size_t nBytes)												\
{																							\
	moduleLogFlags = Rococo::Memory::GetAllocatorLogFlags();								\
	moduleAllocatorFunctions = Rococo::Memory::GetDefaultAllocators();						\
	return moduleAllocatorFunctions.Allocate(nBytes);										\
}																							\
																							\
static void FirstTimeDeallocator(void* buffer)												\
{																							\
	moduleLogFlags = Rococo::Memory::GetAllocatorLogFlags();								\
	moduleAllocatorFunctions = Rococo::Memory::GetDefaultAllocators();						\
	return moduleAllocatorFunctions.Free(buffer);											\
}
