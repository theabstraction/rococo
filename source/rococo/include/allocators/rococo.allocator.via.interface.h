#pragma once

// This file requires an entity moduleAllocator, derived from Rococo::IAllocator to be defined before 
// the file is included

namespace Rococo
{
#ifdef _MODULE_ALLOCATOR_AVAILABLE
	template <class T>
	class AllocatorWithInterface
	{
	public:
		using value_type = T;

	public:
		AllocatorWithInterface() {}
		AllocatorWithInterface(const AllocatorWithInterface&) = default;
		AllocatorWithInterface& operator=(const AllocatorWithInterface&) = delete;

		template <class U>
		AllocatorWithInterface(const AllocatorWithInterface<U>) noexcept {}

		template <class _Up> struct rebind { using other = AllocatorWithInterface<_Up>; };

		T* allocate(std::size_t nElements)
		{
			return reinterpret_cast<T*> ( ANON::moduleAllocator->Allocate(nElements * sizeof(T)) );
		}
		void deallocate(T* p, std::size_t n) noexcept
		{
			UNUSED(n);
			ANON::moduleAllocator->FreeData(p); // If you get a complaint here, check the instructions at the start of the file
		}

		template <class T1,	class U>
		friend bool operator==(const AllocatorWithInterface<T1>& x, const AllocatorWithInterface<U>& y) noexcept;

		template <class U> friend class AllocatorWithInterface;
	};

	template <class T, class U>
	inline bool	operator==(const AllocatorWithInterface<T>& x, const AllocatorWithInterface<U>& y) noexcept
	{
		return true;
	}

	template <class T, class U>
	inline bool operator!=(const AllocatorWithInterface<T>& x, const AllocatorWithInterface<U>& y) noexcept
	{
		return !(x == y);
	}
#else
# error "requires _MODULE_ALLOCATOR_AVAILABLE and ANON::moduleAllocator to be defined prior to inclusion"
#endif
}