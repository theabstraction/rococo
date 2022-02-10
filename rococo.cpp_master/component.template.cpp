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
// #BEGIN_INSTANCED#

	struct ComponentVariableTable;

	struct ComponentVariableLife : IComponentLife
	{
		int64 referenceCount = 0;
		EntityIndex entityIndex;
		bool isDeprecated = false;
		
		ComponentVariableTable& table;

		ComponentVariableLife(EntityIndex index, ComponentVariableTable& refTable) :
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

	struct ComponentVariableTable : IComponentTable<IComponentInterface>
	{
		struct ComponentDesc
		{
			IComponentInterface* interfacePointer = nullptr;
		};

		IComponentInterfaceFactory& componentFactory;
		std::unordered_map<EntityIndex, ComponentDesc, EntityIndexHasher, EntityIndexComparer> rows;
		std::list<EntityIndex> deprecatedList;
		AutoFree<IFreeListAllocatorSupervisor> componentAllocator;
		size_t componentSize;

		ComponentVariableTable(IComponentInterfaceFactory& factory) : componentFactory(factory), rows(1024), componentSize(factory.SizeOfConstructedObject())
		{
			componentAllocator = CreateFreeListAllocator(componentSize + sizeof ComponentVariableLife);
		}

		void Free() override
		{
			delete this;
		}

		ComponentVariableLife& GetLife(IComponentInterface& i)
		{
			uint8* objectBuffer = (uint8*)&i;
			return *(ComponentVariableLife*)(objectBuffer + componentSize);
		}

		Ref<IComponentInterface> AddNew(EntityIndex index) override
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
				IComponentInterface* component = componentFactory.ConstructInPlace(pComponentMemory);
				if (component == nullptr)
				{
					Throw(0, "%s: factory.ConstructInPlace returned null");
				}
				i->second.interfacePointer = component;

				uint8* byteBuffer = (uint8*)pComponentMemory;
				auto* lifeSupport = (ComponentVariableLife*)(byteBuffer + componentSize);
				new (lifeSupport) ComponentVariableLife(index, *this);
				return Ref<IComponentInterface>(*component, GetLife(*component));
			}
			catch (...)
			{
				rows.erase(i);
				throw;
			}
		}

		Ref<IComponentInterface> Find(EntityIndex index) override
		{
			auto i = rows.find(index);
			auto& c = i->second;
			auto* pInterfaceBuffer = (uint8*)c.interfacePointer;

			return i != rows.end() ? Ref<IComponentInterface>(*c.interfacePointer, GetLife(*c.interfacePointer)) : Ref<IComponentInterface>();
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

	bool ComponentVariableLife::Deprecate()
	{
		if (!isDeprecated)
		{
			isDeprecated = true;

			table.deprecatedList.push_back(entityIndex);

			return true;
		}

		return false;
	}

// #END_INSTANCED#

	struct AllComponentTables : IComponentTablesSupervisor
	{
// #BEGIN_INSTANCED#
		AutoFree<IComponentTable<IComponentVariable>> componentVariableTable;
// #END_INSTANCED#

		AllComponentTables(ComponentFactories& factories)
		{
// #BEGIN_INSTANCED#
			componentVariableTable = new COMPONENT_IMPLEMENTATION_NAMESPACE::ComponentVariableTable(factories.componentVariableFactory);
// #END_INSTANCED#
		}
// #BEGIN_INSTANCED#

		Ref<IComponentInterface> AddComponentVariable(EntityIndex index, ActiveComponents& ac)
		{
			ac.hasComponentVariable = true;
			return componentVariableTable->AddNew(index);
		}
// #END_INSTANCED#

		void Deprecate(EntityIndex index, const ActiveComponents& ac) override
		{
// #BEGIN_INSTANCED#
			if (ac.hasComponentVariable)
			{
				componentVariableTable->Deprecate(index);
			}
// #END_INSTANCED#
		}
// #BEGIN_INSTANCED#

		IComponentTable<IComponentVariable>& GetComponentVariableTable() override
		{
			return *componentVariableTable;
		}
// #END_INSTANCED#

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


