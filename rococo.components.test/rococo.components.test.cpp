#include "sys\examples\test.components.h"

#include <rococo.libs.inl>

#include <stdio.h>
#include <new>

#include <rococo.api.h>


using namespace Rococo::Components::Sys;

namespace ANON
{
	struct FireComponent : IFireComponent
	{
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

		void Destruct(IFireComponent* component)
		{
			auto* fire = static_cast<FireComponent*>(component);
			fire->~FireComponent();
		}

		size_t SizeOfConstructedObject() override
		{
			return sizeof FireComponent;
		}
	};

	struct WaterComponent : IWaterComponent
	{
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

		void Destruct(IWaterComponent* component)
		{
			auto* water = static_cast<WaterComponent*>(component);
			water->~WaterComponent();
		}

		size_t SizeOfConstructedObject() override
		{
			return sizeof WaterComponent;
		}
	};
}

using namespace Rococo;
using namespace Rococo::Components;
using namespace Rococo::Components::Sys;

void Validate(bool isTrue, cstr expression, cstr functionName, int line)
{
	if (!isTrue)
	{
		Throw(0, "Validation failure: %s @%s line %d", expression, functionName, line);
	}
}

#define VALIDATE(predicate) Validate(predicate, #predicate, __func__, __LINE__)

void RunTests(IRCObjectTable& ecs)
{
	printf("MaxTableEntries: %u\n", ecs.MaxTableEntries());

	VALIDATE(ecs.ActiveRoidCount() == 0);

	ActiveComponents ac = ecs.GetActiveComponents(ROID());

	VALIDATE(ac.hasFireComponent == false);
	VALIDATE(ac.hasWaterComponent == false);

	ROID roid = ecs.NewROID();
	VALIDATE(roid);

	VALIDATE(ecs.ActiveRoidCount() == 1);

	ac = ecs.GetActiveComponents(roid);

	VALIDATE(ac.hasFireComponent == false);
	VALIDATE(ac.hasWaterComponent == false);

	{
		auto fire = ecs.AddFireComponent(roid);
		fire->Burn();

		ecs.CollectGarbage();

		auto fire2 = ecs.GetFireComponent(roid);
		VALIDATE(fire2);

		fire2->Burn();

		auto fire3 = fire2;

		VALIDATE(fire3.GetRefCount() == 3);
		VALIDATE(fire3.GetRoid() == roid);

		ac = ecs.GetActiveComponents(roid);
		VALIDATE(ac.hasFireComponent == true);

		size_t nIds = ecs.GetFireComponentIDs(nullptr, 0);
		VALIDATE(nIds == 1);

		ROID fireId;
		VALIDATE(1 == ecs.GetFireComponentIDs(&fireId, 1));

		struct ANON: IComponentCallback<IFireComponent>
		{
			int callcount = 0;
			EControlLogic OnComponent(ROID id, IFireComponent& item) override
			{
				VALIDATE(1 == id.index);
				item.Burn();

				callcount++;

				return CONTINUE;
			}
		} cb;

		ecs.EnumerateFireComponents(cb);

		VALIDATE(cb.callcount == 1);

		ecs.DeprecateFireComponent(roid);

		ecs.CollectGarbage();

		ac = ecs.GetActiveComponents(roid);
		VALIDATE(ac.hasFireComponent == false);

		fire->Burn();
	}

	auto fire = ecs.GetFireComponent(roid);
	VALIDATE(!fire);

	ecs.CollectGarbage();
}

int main()
{
	ANON::FireComponentFactory fireFactory;
	ANON::WaterComponentFactory waterFactory;

	ComponentFactories factories{ fireFactory, waterFactory };
	
	AutoFree<IRCObjectTableSupervisor> ecs = Factories::Create_RCO_EntityComponentSystem(factories, 4_megabytes);

	try
	{
		RunTests(*ecs);
	}
	catch (IException& ex)
	{
		printf("%s\n", ex.Message());
	}
	
	return 0;
}
