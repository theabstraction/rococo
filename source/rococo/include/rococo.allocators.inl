#include <rococo.allocators.h>
#include <rococo.debugging.h>
#include <unordered_map>
#include <new>

template <class T>
class std_Malloc_Allocator
{
public:
	using value_type = T;

public:
	std_Malloc_Allocator() {}
	std_Malloc_Allocator(const std_Malloc_Allocator&) = default;
	std_Malloc_Allocator& operator=(const std_Malloc_Allocator&) = delete;

	template <class U>
	std_Malloc_Allocator(const std_Malloc_Allocator<U>) noexcept {}

	template <class _Up> struct rebind { using other = std_Malloc_Allocator<_Up>; };

	T* allocate(std::size_t nElements)
	{
		return reinterpret_cast<T*> (malloc(nElements * sizeof(T)));
	}
	void deallocate(T* p, std::size_t n) noexcept
	{
		UNUSED(n);
		free(p); // If you get a complaint here, check the instructions at the start of the file
	}

	template <class T1, class U>
	friend bool operator==(const std_Malloc_Allocator<T1>& x, const std_Malloc_Allocator<U>& y) noexcept;

	template <class U> friend class std_Malloc_Allocator;
};


template <class T, class U>
inline bool	operator==(const std_Malloc_Allocator<T>& x, const std_Malloc_Allocator<U>& y) noexcept
{
	return true;
}

template <class T, class U>
inline bool operator!=(const std_Malloc_Allocator<T>& x, const std_Malloc_Allocator<U>& y) noexcept
{
	return !(x == y);
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

	template<class T>
	class AllocatorMonitor
	{
	public:
		void Log()
		{
			T t;
			auto& g = t.GetDefaultAllocator();
			g.Log(t.Name(), "Memory stats");
		}

		~AllocatorMonitor()
		{
			T t;
			auto& g = t.GetDefaultAllocator();
			g.Log(t.Name(), "Monitor destruction. Memory stats");
		}
	};

	struct TrackingAtom
	{
		size_t bufferLength = 0;
		bool wasFreed = false;
		int reuseCount = 0;
	};

	template<class T>
	class TrackingAllocator
	{
	public:
		AllocatorMetrics stats;
		std::unordered_map<size_t, TrackingAtom, hash_size_t, size_t_equal_to, std_Malloc_Allocator<std::pair<const size_t,TrackingAtom>>> tracking;

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

			if (buffer)
			{
				auto i = tracking.find((size_t) buffer);
				if (i != tracking.end())
				{
					TrackingAtom& atom = i->second;
					atom.wasFreed = true;
				}
				else
				{
					// This element should not have been freed with this allocator
					OS::TripDebugger();
				}

				stats.usefulFrees++;
				free(buffer);
			}
			else
			{
				stats.blankFrees++;
			}
		}

		void Log(cstr name, cstr intro)
		{
			auto* allocator_printf = Rococo::Debugging::Log;
			Rococo::Memory::Log(stats, name, intro, allocator_printf);

			allocator_printf(" Tracking Metrics\n");
			allocator_printf("  count: %llu\n", tracking.size());

			std::unordered_map<size_t, size_t, hash_size_t, size_t_equal_to, std_Malloc_Allocator<std::pair<const size_t, size_t>>> leakMapSizeToCount;

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
				allocator_printf(" Leaks detected:\n", leakMapSizeToCount.size());
			}

			for(auto i: leakMapSizeToCount)
			{
				allocator_printf("%9llu bytes x %llu\n", i.first, i.second);
			}

			allocator_printf("\n\n");
		}

		using TDefaultMonitor = AllocatorMonitor<T>;
	};

	template<class T>
	class DefaultAllocator
	{
	public:
		AllocatorMetrics stats;

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
				stats.usefulFrees++;
				free(buffer);
			}
			else
			{
				stats.blankFrees++;
			}
		}

		void Log(cstr name, cstr intro)
		{
			Rococo::Memory::Log(stats, name, intro, Rococo::Debugging::Log);
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