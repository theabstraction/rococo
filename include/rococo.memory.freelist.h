#pragma once

#include <vector>

namespace Rococo::Memory
{
	template<class T> class FreeListAllocator
	{
		std::vector<T*> freeList;
		std::vector<T*> allocList;
	public:
		~FreeListAllocator()
		{
			for (auto* p : allocList)
			{
				delete p;
			}
		}

		T* Create()
		{
			if (freeList.empty())
			{
				auto* p = new T();
				allocList.push_back(p);
				return p;
			}
			else
			{
				T* pT = freeList.back();
				freeList.pop_back();

				try
				{
					return new (pT) T();
				}
				catch (...)
				{
					freeList.push_back(pT);
					throw;
				}
			}
		}

		template<typename ...ARGS> T* Create(ARGS... args)
		{
			if (freeList.empty())
			{
				auto* p = new T(args);
				allocList.push_back(p);
				return p;
			}
			else
			{
				T* pT = freeList.back();
				freeList.pop_back();

				try
				{
					return new (pT) T(args);
				}
				catch (...)
				{
					freeList.push_back(pT);
					throw;
				}
			}
		}

		template<typename T, typename ...ARGS> T* Create(T& arg1, ARGS... args)
		{
			if (freeList.empty())
			{
				return new T(arg1, args);
			}
			else
			{
				T* pT = freeList.back();
				freeList.pop_back();

				try
				{
					return new (pT) T(arg1, args);
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