#include "sys\examples\test.components.h"

#ifdef _DEBUG
# pragma comment(lib, "rococo.components.debug.lib")
#else
# pragma comment(lib, "rococo.components.lib")
#endif

#include <rococo.libs.inl>

using namespace Rococo::Components::Sys;

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

	struct WaterComponent : IWaterComponent
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
	using namespace Rococo::Components::Sys::Factories;

	ANON::FireComponentFactory fireFactory;
	ANON::WaterComponentFactory waterFactory;

	ComponentFactories factories{ fireFactory, waterFactory };
	
	AutoFree<IComponentTablesSupervisor> tables = CreateComponentTables(factories);
}
