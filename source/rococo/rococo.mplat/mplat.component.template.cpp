#include <rococo.api.h>
#include <rococo.allocators.h>
#include <list>
#include <unordered_map>
#include "components/rococo.component.entities.h"
#include "mplat.components.h"

#ifdef _DEBUG
#define COMPONENT_IMPLEMENTATION_NAMESPACE ECS
#else
#define COMPONENT_IMPLEMENTATION_NAMESPACE
#endif

namespace COMPONENT_IMPLEMENTATION_NAMESPACE
{
	using namespace Rococo;
	using namespace Rococo::Memory;
	using namespace Rococo::Components;
// #BEGIN_INSTANCED#

	struct ComponentVariableTable;

	struct ComponentVariableLife : IComponentLife
	{
		int64 referenceCount = 0;
		ROID id;
		bool isDeprecated = false;
		
		ComponentVariableTable& table;

		ComponentVariableLife(ROID roid, ComponentVariableTable& refTable) :
			id(roid), table(refTable)
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

		ROID GetRoid() const override
		{
			return id;
		}

		int64 ReleaseRef() override;
		
		// Marks the component as deprecated and returns true if this is the first call that marked it so
		bool Deprecate() override;

		bool IsDeprecated() const override
		{
			return isDeprecated;
		}
	};

	struct ComponentVariableTable
	{
		struct ComponentDesc
		{
			IComponentInterface* interfacePointer = nullptr;
		};

		IComponentFactory<IComponentInterface>& componentFactory;
		std::unordered_map<ROID, ComponentDesc, STDROID, STDROID> rows;
		std::vector<ROID> deprecatedList;
		std::vector<ROID> stubbornList;
		AutoFree<IFreeListAllocatorSupervisor> componentAllocator;
		size_t componentSize;
		int enumLock = 0;

		IRCObjectTableSupervisor& objectTable;

		ComponentVariableTable(IRCObjectTableSupervisor& refObjectTable, IComponentFactory<IComponentInterface>& factory) : 
			objectTable(refObjectTable),
			componentFactory(factory), rows(1024), componentSize(factory.SizeOfConstructedObject())
		{
			componentAllocator = CreateFreeListAllocator(componentSize + sizeof ComponentVariableLife);
		}

		void NotifyOfDeath(ROID id)
		{
			if (enumLock > 0)
			{
				// Cannot delete at this time, wait for the next garbage collection
				deprecatedList.push_back(id);
				return;
			}
			// A component has told us that it is deprecated and a reference to it was released and its reference count is now zero
			auto it = rows.find(id);
			if (it != rows.end())
			{
				auto& component = it->second;
				
				componentFactory.Destruct(component.interfacePointer);
				componentAllocator->FreeBuffer(component.interfacePointer);
				rows.erase(it);
			}
		}

		Ref<IComponentInterface> AddNew(ROID id)
		{
			if (enumLock > 0)
			{
				Throw(0, "%s failed: the components were locked for enumeration", __func__);
			}

			std::pair<ROID, ComponentDesc> nullItem(id, ComponentDesc());
			auto insertion = rows.insert(nullItem);
			if (!insertion.second)
			{
				Throw(0, "%s: a component with the given id 0x%8.8X already exists", __FUNCTION__, id.index);
			}

			auto i = insertion.first;

			try
			{
				void* pComponentMemory = componentAllocator->AllocateBuffer();
				IComponentInterface* component = componentFactory.ConstructInPlace(pComponentMemory);
				if (component == nullptr)
				{
					Throw(0, "%s: factory.ConstructInPlace returned null", __FUNCTION__);
				}
				i->second.interfacePointer = component;

				uint8* byteBuffer = (uint8*)pComponentMemory;
				auto* lifeSupport = (ComponentVariableLife*)(byteBuffer + componentSize);
				new (lifeSupport) ComponentVariableLife(id, *this);
				return Ref<IComponentInterface>(*component, GetLife(*component));
			}
			catch (...)
			{
				rows.erase(i);
				throw;
			}
		}

		void CollectGarbage()
		{
			if (enumLock > 0)
			{
				Throw(0, "%s: An attempt was made to collect garbage during an enumeration lock.", __FUNCTION__);
			}

			while (!deprecatedList.empty())
			{
				ROID id = deprecatedList.back();
				deprecatedList.pop_back();

				NotifyOfDeath(id);
			}
		}
		
		void Deprecate(ROID id)
		{
			auto i = rows.find(id);
			if (i != rows.end())
			{
				auto& component = i->second;
				auto& life = GetLife(*component.interfacePointer);
				life.Deprecate();
			}
		}

		void ForEachComponentVariable(IComponentCallback<IComponentInterface>& cb)
		{
			enumLock++;
			try
			{
				for (auto& row : rows)
				{
					if (cb.OnComponent(row.first, *row.second.interfacePointer) == EFlowLogic::BREAK)
					{
						break;
					}
				}
			}
			catch (...)
			{
				enumLock--;
				throw;
			}
			enumLock--;
		}

		void ForEachComponentVariable(Rococo::Function<EFlowLogic(ROID roid, IComponentInterface&)> functor)
		{
			enumLock++;
			try
			{
				for (auto& row : rows)
				{
					if (functor.Invoke(row.first, *row.second.interfacePointer) == EFlowLogic::BREAK)
					{
						break;
					}
				}
			}
			catch (...)
			{
				enumLock--;
				throw;
			}
			enumLock--;
		}

		Ref<IComponentInterface> Find(ROID id)
		{
			auto i = rows.find(id);
			auto& c = i->second;
			return i != rows.end() ? Ref<IComponentInterface>(*c.interfacePointer, GetLife(*c.interfacePointer)) : Ref<IComponentInterface>();
		}

		size_t GetComponentVariableIDs(ROID* roidOutput, size_t nElementsInOutput)
		{
			if (roidOutput != nullptr)
			{
				size_t nElements = min(nElementsInOutput, rows.size());

				auto* roid = roidOutput;
				auto* rend = roid + nElements;

				for (auto row : rows)
				{
					if (roid == rend) break;
					*roid++ = row.first;
				}

				return nElements;
			}

			return rows.size();
		}


		ComponentVariableLife& GetLife(IComponentInterface& i)
		{
			uint8* objectBuffer = (uint8*)&i;
			return *(ComponentVariableLife*)(objectBuffer + componentSize);
		}
	};

	int64 ComponentVariableLife::ReleaseRef()
	{
		int64 rc = --referenceCount;
		if (rc <= 0 && isDeprecated)
		{
			table.NotifyOfDeath(id);
		}
		return rc;
	}

	bool ComponentVariableLife::Deprecate()
	{
		if (!isDeprecated)
		{
			isDeprecated = true;

			if (referenceCount == 0)
			{
				table.NotifyOfDeath(id);
			}

			return true;
		}

		return false;
	}

// #END_INSTANCED#

	struct AllComponentTables
	{
		int dummy;
		IRCObjectTableSupervisor& objectTable;
// #BEGIN_INSTANCED#
		ComponentVariableTable componentVariableTable;
// #END_INSTANCED#

		AllComponentTables(IRCObjectTableSupervisor& refObjectTable, ComponentFactories& factories):
			dummy(0)
			,objectTable(refObjectTable)
		// #BEGIN_INSTANCED#
			,componentVariableTable(objectTable, factories.componentVariableFactory)
		// #END_INSTANCED#

		{
		}
// #BEGIN_INSTANCED#

		Ref<IComponentInterface> AddComponentVariable(ROID id, ActiveComponents& ac)
		{
			ac.hasComponentVariable = true;
			return componentVariableTable.AddNew(id);
		}
// #END_INSTANCED#

		void Deprecate(ROID id, const ActiveComponents& ac)
		{
// #BEGIN_INSTANCED#
			if (ac.hasComponentVariable)
			{
				componentVariableTable.Deprecate(id);
			}
// #END_INSTANCED#
		}
	};

#pragma pack(push,4)
	struct RCObject
	{
		enum { DEPRECATED };
		RCObject(): salt(DEPRECATED)
		{
			ac = { 0 };
		}

		ROID_SALT salt;			// Gives a version number of the object.
		uint32 activeIdIndex = (uint32) - 1;	// Specifies which activeId slot references this object
		ActiveComponents ac;	// Specifies which components are active for this object

		bool Exists() const
		{
			return salt != DEPRECATED;
		}
	};
#pragma pack(pop)

	struct RCObjectTable: IRCObjectTableSupervisor
	{
		std::vector<RCObject> handleTable;
		std::vector<ROID> freeIds;
		std::vector<ROID_TABLE_INDEX> activeIds;
		std::vector<ROID> deprecationList;

		uint32 maxTableEntries;
		uint32 enumLock = 0;

		AllComponentTables components;
 
		RCObjectTable(ComponentFactories& factories, uint64 maxTableSizeInBytes) :
			components(*this, factories)
		{
			static_assert(sizeof ROID == sizeof uint64);

			uint64 maxTableSize = max(128ULL, maxTableSizeInBytes / (sizeof RCObject + sizeof ROID + sizeof ROID_TABLE_INDEX));

			this->maxTableEntries = maxTableSize > 4000'000ULL ? 4000'000 : (uint32) maxTableSize;

			freeIds.reserve(this->maxTableEntries);

			activeIds.reserve(maxTableEntries);

			for (uint32 i = maxTableEntries - 1; i > 0; i--)
			{
				ROID roid;
				roid.index = i;
				roid.salt = 1;
				freeIds.push_back(roid);
			}

			handleTable.resize(maxTableEntries);
		}

		size_t ActiveRoidCount() const override
		{
			return activeIds.size();
		}

// #BEGIN_INSTANCED#
		Ref<IComponentInterface> AddComponentVariable(ROID id)
		{
			if (id.index > 0 && id.index < maxTableEntries)
			{
				RCObject& object = handleTable[id.index];
				if (id.salt == object.salt)
				{
					return components.AddComponentVariable(id, object.ac);
				}
			}

			return Ref<IComponentInterface>();
		}
// #END_INSTANCED#
		void CollectGarbage() override
		{
			if (enumLock > 0)
			{
				Throw(0, "%s: Cannot collect garbage - the ECS is locked for enumeration.", __FUNCTION__);
			}
// #BEGIN_INSTANCED#
			components.componentVariableTable.CollectGarbage();
// #END_INSTANCED#

			for (ROID id : deprecationList)
			{
				Deprecate(id);
			}

			deprecationList.clear();
		}

		bool Deprecate(ROID roid) override
		{
			if (roid.index == 0 || roid.index >= maxTableEntries)
			{
				return false;
			}

			RCObject& object = handleTable[roid.index];
			if (object.salt == roid.salt)
			{
				if (enumLock > 0)
				{
					// The caller is currently enumerating the roids, which means we cannot immediately delete the roid.
					deprecationList.push_back(roid);
					return false;
				}

				components.Deprecate(roid, object.ac);

				object.salt = RCObject::DEPRECATED;

				if (activeIds.size() == 1)
				{
					activeIds.clear();
				}
				else
				{
					// Move the last activeId to the deleted position
					ROID_TABLE_INDEX lastRoidIndex = activeIds.back();
					handleTable[lastRoidIndex].activeIdIndex = object.activeIdIndex;
					activeIds[object.activeIdIndex] = lastRoidIndex;

					// And delete the last slot
					activeIds.pop_back();
				}

				object.activeIdIndex = (uint32) - 1;

				ROID nextRoid;
				nextRoid.index = roid.index;
				nextRoid.salt = object.salt + 1;

				nextRoid.salt = max(1U, nextRoid.salt);

				freeIds.push_back(nextRoid);
				return true;
			}

			return false;
		}

		void DeprecateAll() override
		{
			for (uint32 i = 1; i < maxTableEntries; ++i)
			{
				auto& object = handleTable[i];
				if (object.Exists())
				{
					ROID id;
					id.index = i;
					id.salt = object.salt;
					Deprecate(id);
				}
			}
		}

// #BEGIN_INSTANCED#
		bool DeprecateComponentVariable(ROID id)
		{
			if (id.index > 0 && id.index < maxTableEntries)
			{
				RCObject& object = handleTable[id.index];
				if (id.salt == object.salt)
				{
					if (object.ac.hasComponentVariable)
					{
						components.componentVariableTable.Deprecate(id);
						object.ac.hasComponentVariable = false;
						return true;
					}
				}
			}

			return false;
		}
// #END_INSTANCED#

		void Enumerate(IROIDCallback& cb) override
		{
			enumLock++;

			try
			{
				// Use an index to address the id - this allows the array to grow in size during enumeration without invalidating the iterator
				size_t maxCount = activeIds.size();
				for (size_t i = 0; i < maxCount; i++)
				{
					ROID_TABLE_INDEX index = activeIds[i];

					ROID id;
					id.salt = handleTable[index].salt;
					id.index = index;
					if (cb.OnROID(id) == EFlowLogic::BREAK)
					{
						break;
					}
				}
			}
			catch (...)
			{
				enumLock--;
				throw;
			}

			enumLock--;

			// The callback may have tried to deprecate a ROID. So that iterators are not invalidated we do this after the outermost enumeration is complete

			if (enumLock == 0 && !deprecationList.empty())
			{
				for (ROID id : deprecationList)
				{
					Deprecate(id);
				}

				deprecationList.clear();
			}
		}
// #BEGIN_INSTANCED#
		void ForEachComponentVariable(IComponentCallback<IComponentInterface>& cb)
		{
			components.componentVariableTable.ForEachComponentVariable(cb);
		}

		void ForEachComponentVariable(Function<EFlowLogic(ROID id, IComponentInterface& component)> functor)
		{
			components.componentVariableTable.ForEachComponentVariable(functor);
		}
// #END_INSTANCED#
// #BEGIN_INSTANCED#

		Ref<IComponentInterface> GetComponentVariable(ROID id)
		{
			if (id.index > 0 && id.index < maxTableEntries)
			{
				RCObject& object = handleTable[id.index];
				if (id.salt == object.salt)
				{
					if (object.ac.hasComponentVariable)
					{
						return components.componentVariableTable.Find(id);
					}
				}
			}

			return Ref<IComponentInterface>();
		}
// #END_INSTANCED#
// #BEGIN_INSTANCED#

		size_t GetComponentVariableIDs(ROID* roidOutput, size_t nElementsInOutput) override
		{
			return components.componentVariableTable.GetComponentVariableIDs(roidOutput, nElementsInOutput);
		}
// #END_INSTANCED#
		bool IsActive(ROID id) const override
		{
			if (id.index == 0 || id.index >= maxTableEntries)
			{
				return false;
			}

			const RCObject& object = handleTable[id.index];
			return (id.salt == object.salt);
		}

		ActiveComponents GetActiveComponents(ROID id) const override
		{
			if (id.index > 0 && id.index < maxTableEntries)
			{
				const RCObject& object = handleTable[id.index];
				if (id.salt == object.salt)
				{
					return object.ac;
				}
			}

			return ActiveComponents{ 0 };
		}

		uint32 MaxTableEntries() const
		{
			return maxTableEntries;
		}

		ROID NewROID() override
		{
			if (freeIds.empty())
			{
				Throw(0, "RCObjectTable is full with %u entries", maxTableEntries);
			}

			ROID newId = freeIds.back();
			freeIds.pop_back();

			RCObject& object = handleTable[newId.index];
			object.salt = newId.salt;
			object.ac = { 0 };

			object.activeIdIndex = (uint32) activeIds.size();
			activeIds.push_back(newId.index);

			return newId;
		}

		void Free() override
		{
			delete this;
		}
	};
} // COMPONENT_IMPLEMENTATION_NAMESPACE

namespace Rococo::Components::Factories
{
	IRCObjectTableSupervisor* Create_RCO_EntityComponentSystem(ComponentFactories& factories, uint64 maxSizeInBytes)
	{
		return new COMPONENT_IMPLEMENTATION_NAMESPACE::RCObjectTable(factories, maxSizeInBytes);
	}
}
