#include <rococo.api.h>
#include <vector>

namespace
{
	using namespace Rococo;

	struct FreeListAllocator: Rococo::IFreeListAllocatorSupervisor
	{
		size_t elementSize;

		std::vector<void*> freeList;

		FreeListAllocator(size_t varElementSize): elementSize(varElementSize)
		{

		}

		void* AllocateBuffer() override
		{
			if (freeList.empty())
			{
				return new char[elementSize];
			}
			else
			{
				void* lastElement = freeList.back();
				freeList.pop_back();
				return lastElement;
			}
		}

		void FreeBuffer(void* buffer) override
		{
			if (buffer == nullptr) return;
			freeList.push_back(buffer);
		}

		void Free() override
		{
			delete this;
		}
	};
}

namespace Rococo
{
	IFreeListAllocatorSupervisor* CreateFreeListAllocator(size_t elementSize)
	{
		return new FreeListAllocator(elementSize);
	}
}