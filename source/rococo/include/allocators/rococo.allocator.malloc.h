#pragma once

// This file requires an entity moduleAllocator, derived from Rococo::IAllocator to be defined before 
// the file is included

namespace Rococo
{
	template <class T>
	class AllocatorWithMalloc
	{
	public:
		using value_type = T;

	public:
		AllocatorWithMalloc() {}
		AllocatorWithMalloc(const AllocatorWithMalloc&) = default;
		AllocatorWithMalloc& operator=(const AllocatorWithMalloc&) = delete;

		template <class U>
		AllocatorWithMalloc(const AllocatorWithMalloc<U>) noexcept {}

		template <class _Up> struct rebind { using other = AllocatorWithMalloc<_Up>; };

		T* allocate(std::size_t nElements)
		{
			return (T*) malloc(nElements * sizeof(T));
		}
		void deallocate(T* p, std::size_t n) noexcept
		{
			UNUSED(n);
			free(p);
		}

		template <class T1, class U>
		friend bool operator==(const AllocatorWithMalloc<T1>& x, const AllocatorWithMalloc<U>& y) noexcept;

		template <class U> friend class AllocatorWithMalloc;
	};

	template <class T, class U>
	inline bool	operator==(const AllocatorWithMalloc<T>& x, const AllocatorWithMalloc<U>& y) noexcept
	{
		return true;
	}

	template <class T, class U>
	inline bool operator!=(const AllocatorWithMalloc<T>& x, const AllocatorWithMalloc<U>& y) noexcept
	{
		return !(x == y);
	}
}