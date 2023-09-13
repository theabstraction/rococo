#include "sys\examples\test.components.h"
#include <rococo.component.entities.h>
#include <rococo.libs.inl>

#include <stdio.h>
#include <new>

#include <rococo.api.h>


using namespace Rococo::Components;

namespace ANON
{
	struct FireComponent : IFireComponent
	{
		void Burn() override
		{
			printf("Fire!");
		}
	};

	struct FireComponentFactory : IComponentFactory<IFireComponent>
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

		size_t SizeOfConstructedObject() const override
		{
			return sizeof FireComponent;
		}

		void Free() override
		{

		}
	};

	struct WaterComponent : IWaterComponent
	{
		void Flood() override
		{
			printf("Water!");
		}
	};

	struct WaterComponentFactory : IComponentFactory<IWaterComponent>
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

		size_t SizeOfConstructedObject() const override
		{
			return sizeof WaterComponent;
		}

		void Free() override
		{

		}
	};
}

using namespace Rococo;
using namespace Rococo::Components;

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
		VALIDATE(fire3.Roid() == roid);

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
