#include "test.components.h"
// Generated at: Feb 10 2022 P UTC
// Based on the template file: C:\work\rococo\rococo.cpp_master\component.template.cpp

#include <rococo.api.h>
#include <list>
#include <unordered_map>
#include "rococo.component.entities.h"

#define COMPONENT_IMPLEMENTATION_NAMESPACE

namespace COMPONENT_IMPLEMENTATION_NAMESPACE
{
	using namespace Rococo;
	using namespace Rococo::Components;
	using namespace Rococo::Components::Sys;

	struct FireComponentTable;

	struct FireComponentLife : IComponentLife
	{
		int64 referenceCount = 0;
		EntityIndex entityIndex;
		bool isDeprecated = false;
		
		FireComponentTable& table;

		FireComponentLife(EntityIndex index, FireComponentTable& refTable) :
			entityIndex(index), table(refTable)
		{

		}

		int64 AddRef() override
		{
			return ++referenceCount;
		}

		int64 GetRefCount() const override
		{
			return referenceCount;
		}

		int64 ReleaseRef() override
		{
			return --referenceCount;
		}

		// Marks the component as deprecated and returns true if this is the first call that marked it so
		bool Deprecate() override;

		bool IsDeprecated() const override
		{
			return isDeprecated;
		}
	};

	struct FireComponentTable : IComponentTable<IFireComponent>
	{
		struct ComponentDesc
		{
			IFireComponent* interfacePointer = nullptr;
		};

		IFireComponentFactory& componentFactory;
		std::unordered_map<EntityIndex, ComponentDesc, EntityIndexHasher, EntityIndexComparer> rows;
		std::list<EntityIndex> deprecatedList;
		AutoFree<IFreeListAllocatorSupervisor> componentAllocator;
		size_t componentSize;

		FireComponentTable(IFireComponentFactory& factory) : componentFactory(factory), rows(1024), componentSize(factory.SizeOfConstructedObject())
		{
			componentAllocator = CreateFreeListAllocator(componentSize + sizeof FireComponentLife);
		}

		void Free() override
		{
			delete this;
		}

		FireComponentLife& GetLife(IFireComponent& i)
		{
			uint8* objectBuffer = (uint8*)&i;
			return *(FireComponentLife*)(objectBuffer + componentSize);
		}

		Ref<IFireComponent> AddNew(EntityIndex index) override
		{
			std::pair<EntityIndex, ComponentDesc> nullItem(index, ComponentDesc());
			auto insertion = rows.insert(nullItem);
			if (!insertion.second)
			{
				Throw(0, "%s: a component with the given id already exists", __FUNCTION__, index.id);
			}

			auto i = insertion.first;

			try
			{
				void* pComponentMemory = componentAllocator->AllocateBuffer();
				IFireComponent* component = componentFactory.ConstructInPlace(pComponentMemory);
				if (component == nullptr)
				{
					Throw(0, "%s: factory.ConstructInPlace returned null");
				}
				i->second.interfacePointer = component;

				uint8* byteBuffer = (uint8*)pComponentMemory;
				auto* lifeSupport = (FireComponentLife*)(byteBuffer + componentSize);
				new (lifeSupport) FireComponentLife(index, *this);
				return Ref<IFireComponent>(*component, GetLife(*component));
			}
			catch (...)
			{
				rows.erase(i);
				throw;
			}
		}

		Ref<IFireComponent> Find(EntityIndex index) override
		{
			auto i = rows.find(index);
			auto& c = i->second;
			auto* pInterfaceBuffer = (uint8*)c.interfacePointer;

			return i != rows.end() ? Ref<IFireComponent>(*c.interfacePointer, GetLife(*c.interfacePointer)) : Ref<IFireComponent>();
		}

		void Deprecate(EntityIndex index) override
		{
			auto i = rows.find(index);
			if (i != rows.end())
			{
				auto& component = i->second;
				auto& life = GetLife(*component.interfacePointer);
				life.Deprecate();
			}
		}

		void Flush() override
		{
			auto i = deprecatedList.begin();
			while (i != deprecatedList.end())
			{
				auto it = rows.find(*i);
				if (it != rows.end())
				{
					auto& component = it->second;
					auto& life = GetLife(*component.interfacePointer);
					if (life.isDeprecated && life.referenceCount <= 0)
					{
						i = deprecatedList.erase(i);
						component.interfacePointer->Free();
					}
					else
					{
						i++;
					}
				}
				else
				{
					i = deprecatedList.erase(i);
				}
			}
		}
	};

	bool FireComponentLife::Deprecate()
	{
		if (!isDeprecated)
		{
			isDeprecated = true;

			table.deprecatedList.push_back(entityIndex);

			return true;
		}

		return false;
	}


	struct WaterComponentTable;

	struct WaterComponentLife : IComponentLife
	{
		int64 referenceCount = 0;
		EntityIndex entityIndex;
		bool isDeprecated = false;
		
		WaterComponentTable& table;

		WaterComponentLife(EntityIndex index, WaterComponentTable& refTable) :
			entityIndex(index), table(refTable)
		{

		}

		int64 AddRef() override
		{
			return ++referenceCount;
		}

		int64 GetRefCount() const override
		{
			return referenceCount;
		}

		int64 ReleaseRef() override
		{
			return --referenceCount;
		}

		// Marks the component as deprecated and returns true if this is the first call that marked it so
		bool Deprecate() override;

		bool IsDeprecated() const override
		{
			return isDeprecated;
		}
	};

	struct WaterComponentTable : IComponentTable<IWaterComponent>
	{
		struct ComponentDesc
		{
			IWaterComponent* interfacePointer = nullptr;
		};

		IWaterComponentFactory& componentFactory;
		std::unordered_map<EntityIndex, ComponentDesc, EntityIndexHasher, EntityIndexComparer> rows;
		std::list<EntityIndex> deprecatedList;
		AutoFree<IFreeListAllocatorSupervisor> componentAllocator;
		size_t componentSize;

		WaterComponentTable(IWaterComponentFactory& factory) : componentFactory(factory), rows(1024), componentSize(factory.SizeOfConstructedObject())
		{
			componentAllocator = CreateFreeListAllocator(componentSize + sizeof WaterComponentLife);
		}

		void Free() override
		{
			delete this;
		}

		WaterComponentLife& GetLife(IWaterComponent& i)
		{
			uint8* objectBuffer = (uint8*)&i;
			return *(WaterComponentLife*)(objectBuffer + componentSize);
		}

		Ref<IWaterComponent> AddNew(EntityIndex index) override
		{
			std::pair<EntityIndex, ComponentDesc> nullItem(index, ComponentDesc());
			auto insertion = rows.insert(nullItem);
			if (!insertion.second)
			{
				Throw(0, "%s: a component with the given id already exists", __FUNCTION__, index.id);
			}

			auto i = insertion.first;

			try
			{
				void* pComponentMemory = componentAllocator->AllocateBuffer();
				IWaterComponent* component = componentFactory.ConstructInPlace(pComponentMemory);
				if (component == nullptr)
				{
					Throw(0, "%s: factory.ConstructInPlace returned null");
				}
				i->second.interfacePointer = component;

				uint8* byteBuffer = (uint8*)pComponentMemory;
				auto* lifeSupport = (WaterComponentLife*)(byteBuffer + componentSize);
				new (lifeSupport) WaterComponentLife(index, *this);
				return Ref<IWaterComponent>(*component, GetLife(*component));
			}
			catch (...)
			{
				rows.erase(i);
				throw;
			}
		}

		Ref<IWaterComponent> Find(EntityIndex index) override
		{
			auto i = rows.find(index);
			auto& c = i->second;
			auto* pInterfaceBuffer = (uint8*)c.interfacePointer;

			return i != rows.end() ? Ref<IWaterComponent>(*c.interfacePointer, GetLife(*c.interfacePointer)) : Ref<IWaterComponent>();
		}

		void Deprecate(EntityIndex index) override
		{
			auto i = rows.find(index);
			if (i != rows.end())
			{
				auto& component = i->second;
				auto& life = GetLife(*component.interfacePointer);
				life.Deprecate();
			}
		}

		void Flush() override
		{
			auto i = deprecatedList.begin();
			while (i != deprecatedList.end())
			{
				auto it = rows.find(*i);
				if (it != rows.end())
				{
					auto& component = it->second;
					auto& life = GetLife(*component.interfacePointer);
					if (life.isDeprecated && life.referenceCount <= 0)
					{
						i = deprecatedList.erase(i);
						component.interfacePointer->Free();
					}
					else
					{
						i++;
					}
				}
				else
				{
					i = deprecatedList.erase(i);
				}
			}
		}
	};

	bool WaterComponentLife::Deprecate()
	{
		if (!isDeprecated)
		{
			isDeprecated = true;

			table.deprecatedList.push_back(entityIndex);

			return true;
		}

		return false;
	}


	struct AllComponentTables : IComponentTablesSupervisor
	{
		AutoFree<IComponentTable<IFireComponent>> fireComponentTable;
		AutoFree<IComponentTable<IWaterComponent>> waterComponentTable;

		AllComponentTables(ComponentFactories& factories)
		{
			fireComponentTable = new COMPONENT_IMPLEMENTATION_NAMESPACE::FireComponentTable(factories.fireComponentFactory);
			waterComponentTable = new COMPONENT_IMPLEMENTATION_NAMESPACE::WaterComponentTable(factories.waterComponentFactory);
		}

		Ref<IFireComponent> AddFireComponent(EntityIndex index, ActiveComponents& ac)
		{
			ac.hasFireComponent = true;
			return fireComponentTable->AddNew(index);
		}

		Ref<IWaterComponent> AddWaterComponent(EntityIndex index, ActiveComponents& ac)
		{
			ac.hasWaterComponent = true;
			return waterComponentTable->AddNew(index);
		}

		void Deprecate(EntityIndex index, const ActiveComponents& ac) override
		{
			if (ac.hasFireComponent)
			{
				fireComponentTable->Deprecate(index);
			}
			if (ac.hasWaterComponent)
			{
				waterComponentTable->Deprecate(index);
			}
		}

		IComponentTable<IFireComponent>& GetFireComponentTable() override
		{
			return *fireComponentTable;
		}

		IComponentTable<IWaterComponent>& GetWaterComponentTable() override
		{
			return *waterComponentTable;
		}

		void Free() override
		{
			delete this;
		}
	};
} // COMPONENT_IMPLEMENTATION_NAMESPACE

namespace Rococo::Components::Sys::Factories
{
	IComponentTablesSupervisor* CreateComponentTables(ComponentFactories& factories)
	{
		return new COMPONENT_IMPLEMENTATION_NAMESPACE::AllComponentTables(factories);
	}
}



