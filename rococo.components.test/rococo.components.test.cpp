#include "sys\examples\test.components.h"

#ifdef _DEBUG
# pragma comment(lib, "rococo.components.debug.lib")
#else
# pragma comment(lib, "rococo.components.lib")
#endif

#include <rococo.libs.inl>

#include <stdio.h>
#include <new>

using namespace Rococo::Components::Sys;

namespace ANON
{
	struct FireComponent : IFireComponent
	{
		void Free() override
		{
			delete this;
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

	struct WaterComponent : IWaterComponent
	{
		void Free() override
		{
			delete this;
		}

		void Flood() override
		{
			printf("Water!");
		}
	};

	struct WaterComponentFactory : IWaterComponentFactory
	{
		IWaterComponent* ConstructInPlace(void* pMemory) override
		{
			return new (pMemory) WaterComponent();
		}

		size_t SizeOfConstructedObject() override
		{
			return sizeof WaterComponent;
		}
	};
}


int main()
{
	using namespace Rococo;
	using namespace Rococo::Components::Sys;

	ANON::FireComponentFactory fireFactory;
	ANON::WaterComponentFactory waterFactory;

	ComponentFactories factories{ fireFactory, waterFactory };
	
	AutoFree<IComponentTablesSupervisor> tables = Factories::CreateComponentTables(factories);
}
