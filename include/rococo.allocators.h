#ifndef ROCOCO_ALLOCATORS_H
#define ROCOCO_ALLOCATORS_H

#include <vector>

namespace Rococo
{
	namespace Memory
	{
		// HomogenousAllocator<T> provides constant time allocation and freeing of memory for class T
		// and should protect against rapid reallocation of such objects from causing memory fragmentation
		template<class T> class HomogenousAllocator
		{
			std::vector<T*> freeAllocs;
			std::vector<T*> activeAllocs;
		public:
			~HomogenousAllocator()
			{
				for (auto i : activeAllocs)
				{
					_aligned_free(i);
				}
			}

			T* Allocate()
			{
				if (freeAllocs.empty())
				{
					void* buffer = _aligned_malloc(sizeof(T), 16);
					auto t = (T*)buffer;
					activeAllocs.push_back(t);
					return t;
				}
				else
				{
					T* t = freeAllocs.back();
					freeAllocs.pop_back();
					return t;
				}
			}

			void Free(T* t)
			{
				if (t)
				{
					freeAllocs.push_back(t);
				}
			}

			size_t NumberOfFreeItems() const
			{
				return freeAllocs.size();
			}

			size_t NumberOfAllocations() const
			{
				return activeAllocs.size();
			}
		};
	}
}

#endif