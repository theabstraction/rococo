#pragma once

#include <rococo.os.h>
#include <rococo.allocators.h>
#include <rococo.debugging.h>
#include <unordered_map>
#include <new>

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


namespace Rococo::Memory
{
	struct voids_equal_to 
	{
		[[nodiscard]] constexpr bool operator()(const void* a, const void* b) const
		{
			return a == b;
		}
	};

	struct size_t_equal_to
	{
		[[nodiscard]] constexpr bool operator()(const size_t a, const size_t b) const
		{
			return a == b;
		}
	};

	struct hash_size_t
	{
		size_t operator() (const size_t val) const
		{
			return val;
		}
	};

	// The default allocator monitor, invokes the global allocator log if GetAllocatorLogFlags().Metrics_On_Module_Exit evaluates to true
	template<class T>
	class AllocatorMonitor
	{
	public:
		AllocatorMonitor()
		{

		}

		void Log()
		{
			T t;
			auto& g = t.GetDefaultAllocator();
			g.Log(t.Name(), "Memory stats");
		}

		~AllocatorMonitor()
		{
			if (GetAllocatorLogFlags().LogOnModuleExit)
			{
				T t;
				auto& g = t.GetDefaultAllocator();
				g.Log(t.Name(), "Monitor destruction. Memory stats");
			}
		}
	};

	struct TrackingAtom
	{
		size_t bufferLength = 0;
		bool wasFreed = false;
		int reuseCount = 0;
	};

	// Uses the rococo libraries' default allocator found in Rococo::Utils.
	template <class T>
	class RococoUtilsAllocator
	{
	public:
		using value_type = T;
		
	public:
		RococoUtilsAllocator() {}
		RococoUtilsAllocator(const RococoUtilsAllocator&) = default;
		RococoUtilsAllocator& operator=(const RococoUtilsAllocator&) = delete;

		template <class U>
		RococoUtilsAllocator(const RococoUtilsAllocator<U>) noexcept {}

		template <class _Up> struct rebind { using other = RococoUtilsAllocator<_Up>; };

		T* allocate(std::size_t nElements)
		{
			return reinterpret_cast<T*> (moduleAllocatorFunctions.Allocate(nElements * sizeof(T)));
		}
		void deallocate(T* p, std::size_t n) noexcept
		{
			UNUSED(n);
			moduleAllocatorFunctions.Free(p); // If you get a complaint here, check the instructions at the start of the file
		}

		template <class T1, class U>
		friend bool operator==(const RococoUtilsAllocator<T1>& x, const RococoUtilsAllocator<U>& y) noexcept;

		template <class U> friend class RococoUtilsAllocator;
	};

	template <class T, class U>
	inline bool	operator==(const RococoUtilsAllocator<T>& x, const RococoUtilsAllocator<U>& y) noexcept
	{
		return true;
	}

	template <class T, class U>
	inline bool operator!=(const RococoUtilsAllocator<T>& x, const RococoUtilsAllocator<U>& y) noexcept
	{
		return !(x == y);
	}

	// Used to track leaks, bad deallocs and consolidate the costs of allocations
	template<class T>
	class TrackingAllocator
	{
	public:
		size_t totalFreed = 0;
		AllocatorMetrics stats;
		std::unordered_map<size_t, TrackingAtom, hash_size_t, size_t_equal_to, RococoUtilsAllocator<std::pair<const size_t,TrackingAtom>>> tracking;
		AutoFree<OS::ICriticalSection> criticalSection;

		TrackingAllocator()
		{
			criticalSection = OS::CreateCriticalSection(OS::CriticalSectionMemorySource::GLOBAL_MALLOC);
		}

		void* ModuleAllocate(std::size_t nBytes)
		{
			stats.totalAllocationSize += nBytes;
			stats.totalAllocations++;

			auto* data = moduleAllocatorFunctions.Allocate(nBytes);

			OS::Lock lock(criticalSection);

#ifdef _DEBUG
			memset(data, 0xCD, nBytes);
#endif

			auto i = tracking.find((size_t) data);
			if (i == tracking.end())
			{
				tracking.insert(std::make_pair((const size_t) data, TrackingAtom{ nBytes, false, 0 }));
			}
			else
			{
				TrackingAtom& atom = i->second;
				atom.bufferLength = nBytes;
				atom.reuseCount++;
				atom.wasFreed = false;
			}

			return data;
		}

		void ModuleFree(void* buffer)
		{
			stats.totalFrees++;

			OS::Lock lock(criticalSection);

			if (buffer)
			{
				auto i = tracking.find((size_t) buffer);
				if (i != tracking.end())
				{
					TrackingAtom& atom = i->second;

					if (!atom.wasFreed)
					{
#ifdef _DEBUG
						memset(buffer, 0xDE, atom.bufferLength);
#endif
						totalFreed += atom.bufferLength;
						atom.wasFreed = true;
					}
				}
				else
				{
					// This element should not have been freed with this allocator
					OS::TripDebugger();
				}

				stats.usefulFrees++;
				moduleAllocatorFunctions.Free(buffer);
			}
			else
			{
				stats.blankFrees++;
			}
		}

		void Log(cstr name, cstr intro)
		{
			auto* allocator_printf = Rococo::Debugging::Log;

			OS::Lock lock(criticalSection);

			if (GetAllocatorLogFlags().LogDetailedMetrics)
			{
				Rococo::Memory::Log(stats, name, intro, allocator_printf);

				allocator_printf(" Tracking Metrics\n");
				allocator_printf("  count: %llu\n", tracking.size());
			}

			if (GetAllocatorLogFlags().LogLeaks || OS::IsDebugging())
			{
				std::unordered_map<size_t, size_t, hash_size_t, size_t_equal_to, RococoUtilsAllocator<std::pair<const size_t, size_t>>> leakMapSizeToCount;

				for (auto i : tracking)
				{
					TrackingAtom& atom = i.second;
					if (!atom.wasFreed)
					{
						std::pair<const size_t, size_t> newItem(atom.bufferLength, 1);
						auto j = leakMapSizeToCount.insert(newItem);
						if (!j.second)
						{
							j.first->second++;
						}
					}
				}

				if (!leakMapSizeToCount.empty())
				{
					allocator_printf(" Leaks detected: (%llu bytes)\n", stats.totalAllocationSize - totalFreed);
				}

				for (auto i : leakMapSizeToCount)
				{
					allocator_printf("%9llu bytes x %-9llu = %llu bytes\n", i.first, i.second, i.first * i.second);
				}

				allocator_printf("\n\n");

				if (!leakMapSizeToCount.empty())
				{
					OS::TripDebugger();
				}
			}
		}

		using TDefaultMonitor = AllocatorMonitor<T>;
	};

	// Similar to the default allocator, but it does not monitor any allocation metrics. It is ~ 1-2% faster than the DefaultAllocator
	template<class T>
	class FastAllocator
	{
	public:
		void* ModuleAllocate(std::size_t nBytes)
		{
			return moduleAllocatorFunctions.Allocate(nBytes);
		}

		void ModuleFree(void* buffer)
		{
			moduleAllocatorFunctions.Free(buffer);
		}

		void Log(cstr name, cstr intro)
		{
		}
	};

	template<class T>
	class DefaultAllocator
	{
	public:
		AllocatorMetrics stats;

		inline void* ModuleAllocate(std::size_t nBytes)
		{
			stats.totalAllocationSize += nBytes;
			stats.totalAllocations++;

			return moduleAllocatorFunctions.Allocate(nBytes);
		}

		inline void ModuleFree(void* buffer)
		{
			stats.totalFrees++;
			if (buffer)
			{
				stats.usefulFrees++;
				moduleAllocatorFunctions.Free(buffer);
			}
			else
			{
				stats.blankFrees++;
			}
		}

		void Log(cstr name, cstr intro)
		{
			if (Memory::GetAllocatorLogFlags().LogDetailedMetrics)
			{
				Rococo::Memory::Log(stats, name, intro, Rococo::Debugging::Log);
			}
		}
	};

	template<class T>
	class MallocAllocator
	{
	public:
		void* ModuleAllocate(std::size_t nBytes)
		{
			auto* data = malloc(nBytes);
			if (!data)
			{
				throw std::bad_alloc();
			}
			return data;
		}

		void ModuleFree(void* buffer)
		{
			free(buffer);
		}

		void Log(cstr name, cstr intro)
		{
		}
	};
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////
#define DeclareAllocator(ALLOCATOR, SHORTNAME,GLOBAL_ALLOCATOR_REF)											\
struct SHORTNAME;																							\
Rococo::Memory::ALLOCATOR<SHORTNAME> GLOBAL_ALLOCATOR_REF;													\
struct SHORTNAME																							\
{																											\
	const char* Name()																						\
	{																										\
		return #SHORTNAME;																					\
	}																										\
																											\
	auto& GetDefaultAllocator()										\
	{																										\
		return GLOBAL_ALLOCATOR_REF;																		\
	}																										\
};
/////////////////////////////////////////////////////////////////////////////////////////////////////////////
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

/////////////////////////////////////////////////////////////////////////////////////////////////////////////
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
}