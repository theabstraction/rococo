#pragma once

#include <vector>

namespace Rococo::Memory
{
	template<class T, size_t ALIGN> class FreeListAllocator
	{
		std::vector<T*> freeList;
		std::vector<T*> allocList;
	public:
		~FreeListAllocator()
		{
			for (auto* p : allocList)
			{
				_aligned_free(p);
			}
		}

		size_t MemoryUse() const
		{
			return allocList.size() * (sizeof(void*) + sizeof(T)) + freeList.size() * sizeof(void*);
		}

		template<typename CREATOR> T* CreateWith(CREATOR createAtMem)
		{
			if (freeList.empty())
			{
				T* p = reinterpret_cast<T*>( _aligned_malloc(sizeof(T), ALIGN) );
				createAtMem(p);
				allocList.push_back(p);
				return p;
			}
			else
			{
				T* pT = freeList.back();
				freeList.pop_back();

				try
				{
					return createAtMem(pT);
				}
				catch (...)
				{
					freeList.push_back(pT);
					throw;
				}
			}
		}

		void FreeObject(T* object)
		{
			if (object)
			{
				object->~T();
				freeList.push_back(object);
			}
		}
	};
}