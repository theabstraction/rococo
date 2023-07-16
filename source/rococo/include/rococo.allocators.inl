#include <rococo.allocators.h>
#include <rococo.debugging.h>
#include <new>

namespace Rococo::Memory
{
	template<class T>
	class DefaultAllocator
	{
	public:
		MemoryStats stats;

		void* ModuleAllocate(std::size_t nBytes)
		{
			stats.totalAllocationSize += nBytes;
			stats.totalAllocations++;

			auto* data = malloc(nBytes);
			if (!data)
			{
				stats.failedAllocations++;
				Rococo::Throw(0, "%s(%s): Cannot allocate %llu bytes", __FUNCTION__, nBytes);
			}
			return data;
		}

		void ModuleFree(void* buffer)
		{
			stats.totalFrees++;
			if (buffer)
			{
				free(buffer);
			}
			else
			{
				stats.blankFrees++;
			}
		}
	};

	template<class T>
	class AllocatorMonitor
	{
	public:
		void Log()
		{
			T t;
			auto& g = t.GetDefaultAllocator();
			Rococo::Memory::Log(g.stats, t.Name(), "Memory stats", Rococo::Debugging::Log);
		}

		~AllocatorMonitor()
		{
			T t;
			auto& g = t.GetDefaultAllocator();
			Rococo::Memory::Log(g.stats, t.Name(), "Monitor destruction. Memory stats", Rococo::Debugging::Log);
		}
	};
}

#define OVERRIDE_MODULE_ALLOCATORS_WITH_MODULE_FUNCTIONS()													\
void* ModuleAllocate(std::size_t nBytes);																	\
void ModuleFree(void* buffer);																				\
																											\
_NODISCARD _Ret_notnull_ _Post_writable_byte_size_(nBytes) _VCRT_ALLOCATOR									\
void* __CRTDECL operator new(std::size_t nBytes)															\
{																											\
	return ModuleAllocate(nBytes);																			\
}																											\
																											\
_Ret_maybenull_ _Success_(return != NULL) _Post_writable_byte_size_(nBytes) _VCRT_ALLOCATOR					\
void* __CRTDECL operator new(size_t nBytes, ::std::nothrow_t const&) noexcept								\
{																											\
	try																										\
	{																										\
		return ModuleAllocate(nBytes);																		\
	}																										\
	catch (...)																								\
	{																										\
		return nullptr;																						\
	}																										\
}																											\
																											\
_NODISCARD _Ret_notnull_ _Post_writable_byte_size_(nBytes) _VCRT_ALLOCATOR									\
void* __CRTDECL operator new[](size_t nBytes)																\
{																											\
	return ModuleAllocate(nBytes);																			\
}																											\
																											\
_NODISCARD _Ret_maybenull_ _Success_(return != NULL) _Post_writable_byte_size_(nBytes) _VCRT_ALLOCATOR		\
void* __CRTDECL operator new[](size_t nBytes, ::std::nothrow_t const&) noexcept								\
{																											\
	try																										\
	{																										\
		return ModuleAllocate(nBytes);																		\
	}																										\
	catch (...)																								\
	{																										\
		return nullptr;																						\
	}																										\
}																											\
																											\
void operator delete(void* buffer) throw()																	\
{																											\
	ModuleFree(buffer);																						\
}																											\


#define DeclareDefaultAllocator(SHORTNAME,GLOBAL_ALLOCATOR_REF)												\
struct SHORTNAME;																							\
Rococo::Memory::DefaultAllocator<SHORTNAME> GLOBAL_ALLOCATOR_REF;											\
struct SHORTNAME																							\
{																											\
	const char* Name()																						\
	{																										\
		return #SHORTNAME;																					\
	}																										\
																											\
	Rococo::Memory::DefaultAllocator<SHORTNAME>& GetDefaultAllocator()										\
	{																										\
		return GLOBAL_ALLOCATOR_REF;																		\
	}																										\
};

#define OVERRIDE_MODULE_ALLOCATORS_WITH_FUNCTOR(functor)													\
																											\
_NODISCARD _Ret_notnull_ _Post_writable_byte_size_(nBytes) _VCRT_ALLOCATOR									\
void* __CRTDECL operator new(std::size_t nBytes)															\
{																											\
	return functor.ModuleAllocate(nBytes);																	\
}																											\
																											\
_Ret_maybenull_ _Success_(return != NULL) _Post_writable_byte_size_(nBytes) _VCRT_ALLOCATOR					\
void* __CRTDECL operator new(size_t nBytes, ::std::nothrow_t const&) noexcept								\
{																											\
	try																										\
	{																										\
		return functor.ModuleAllocate(nBytes);																\
	}																										\
	catch (...)																								\
	{																										\
		return nullptr;																						\
	}																										\
}																											\
																											\
_NODISCARD _Ret_notnull_ _Post_writable_byte_size_(nBytes) _VCRT_ALLOCATOR									\
void* __CRTDECL operator new[](size_t nBytes)																\
{																											\
	return functor.ModuleAllocate(nBytes);																	\
}																											\
																											\
_NODISCARD _Ret_maybenull_ _Success_(return != NULL) _Post_writable_byte_size_(nBytes) _VCRT_ALLOCATOR		\
void* __CRTDECL operator new[](size_t nBytes, ::std::nothrow_t const&) noexcept								\
{																											\
	try																										\
	{																										\
		return functor.ModuleAllocate(nBytes);																\
	}																										\
	catch (...)																								\
	{																										\
		return nullptr;																						\
	}																										\
}																											\
																											\
void operator delete(void* buffer) throw()																	\
{																											\
	functor.ModuleFree(buffer);																				\
}																											\