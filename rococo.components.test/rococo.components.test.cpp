#include "sys\examples\fire.component.h"

#ifdef _DEBUG
# pragma comment(lib, "rococo.components.debug.lib")
#else
# pragma comment(lib, "rococo.components.lib")
#endif

#include <rococo.libs.inl>

namespace ANON
{
	struct FireComponent : IFireComponent
	{
		bool isDeprecated = false;

		bool Deprecate() override
		{
			if (isDeprecated)
			{
				return false;
			}
			else
			{
				isDeprecated = true;
				return true;
			}
		}

		void Free() override
		{
			delete this;
		}

		bool IsReadyToDelete() const override
		{
			return isDeprecated;
		}

		void Burn() override
		{
			printf("Fire!");
		}
	};

	struct FireComponentFactory : IFireComponentFactory
	{
		IFireComponent* ConstructInPlace(void* pMemory) override
		{
			return new (pMemory) FireComponent();
		}

		size_t SizeOfConstructedObject() override
		{
			return sizeof FireComponent;
		}
	};
}


int main()
{
	using namespace Rococo;
	using namespace Rococo::Components::Sys;
	using namespace Rococo::Components::Sys::Factories;

	ANON::FireComponentFactory fireFactory;
	
	AutoFree<IFireComponentTable> table = NewComponentInterfaceTable(fireFactory);

	EntityIndex index = { 1,7 };
	auto* fire = table->AddNew(index);
}
