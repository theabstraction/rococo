#include "test.components.h"
// Generated at: Feb 11 2022 P UTC
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
		ROID id;
		bool isDeprecated = false;
		
		FireComponentTable& table;

		FireComponentLife(ROID roid, FireComponentTable& refTable) :
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
		std::unordered_map<ROID, ComponentDesc, STDROID, STDROID> rows;
		std::list<ROID> deprecatedList;
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

		Ref<IFireComponent> AddNew(ROID id) override
		{
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
				IFireComponent* component = componentFactory.ConstructInPlace(pComponentMemory);
				if (component == nullptr)
				{
					Throw(0, "%s: factory.ConstructInPlace returned null");
				}
				i->second.interfacePointer = component;

				uint8* byteBuffer = (uint8*)pComponentMemory;
				auto* lifeSupport = (FireComponentLife*)(byteBuffer + componentSize);
				new (lifeSupport) FireComponentLife(id, *this);
				return Ref<IFireComponent>(*component, GetLife(*component));
			}
			catch (...)
			{
				rows.erase(i);
				throw;
			}
		}

		Ref<IFireComponent> Find(ROID id) override
		{
			auto i = rows.find(id);
			auto& c = i->second;
			auto* pInterfaceBuffer = (uint8*)c.interfacePointer;

			return i != rows.end() ? Ref<IFireComponent>(*c.interfacePointer, GetLife(*c.interfacePointer)) : Ref<IFireComponent>();
		}

		void Deprecate(ROID id) override
		{
			auto i = rows.find(id);
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

			table.deprecatedList.push_back(id);

			return true;
		}

		return false;
	}


	struct WaterComponentTable;

	struct WaterComponentLife : IComponentLife
	{
		int64 referenceCount = 0;
		ROID id;
		bool isDeprecated = false;
		
		WaterComponentTable& table;

		WaterComponentLife(ROID roid, WaterComponentTable& refTable) :
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
		std::unordered_map<ROID, ComponentDesc, STDROID, STDROID> rows;
		std::list<ROID> deprecatedList;
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

		Ref<IWaterComponent> AddNew(ROID id) override
		{
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
				IWaterComponent* component = componentFactory.ConstructInPlace(pComponentMemory);
				if (component == nullptr)
				{
					Throw(0, "%s: factory.ConstructInPlace returned null");
				}
				i->second.interfacePointer = component;

				uint8* byteBuffer = (uint8*)pComponentMemory;
				auto* lifeSupport = (WaterComponentLife*)(byteBuffer + componentSize);
				new (lifeSupport) WaterComponentLife(id, *this);
				return Ref<IWaterComponent>(*component, GetLife(*component));
			}
			catch (...)
			{
				rows.erase(i);
				throw;
			}
		}

		Ref<IWaterComponent> Find(ROID id) override
		{
			auto i = rows.find(id);
			auto& c = i->second;
			auto* pInterfaceBuffer = (uint8*)c.interfacePointer;

			return i != rows.end() ? Ref<IWaterComponent>(*c.interfacePointer, GetLife(*c.interfacePointer)) : Ref<IWaterComponent>();
		}

		void Deprecate(ROID id) override
		{
			auto i = rows.find(id);
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

			table.deprecatedList.push_back(id);

			return true;
		}

		return false;
	}


	struct AllComponentTables : IComponentTables
	{
		AutoFree<IComponentTable<IFireComponent>> fireComponentTable;
		AutoFree<IComponentTable<IWaterComponent>> waterComponentTable;

		AllComponentTables(ComponentFactories& factories)
		{
			fireComponentTable = new COMPONENT_IMPLEMENTATION_NAMESPACE::FireComponentTable(factories.fireComponentFactory);
			waterComponentTable = new COMPONENT_IMPLEMENTATION_NAMESPACE::WaterComponentTable(factories.waterComponentFactory);
		}

		Ref<IFireComponent> AddFireComponent(ROID id, ActiveComponents& ac)
		{
			ac.hasFireComponent = true;
			return fireComponentTable->AddNew(id);
		}

		Ref<IWaterComponent> AddWaterComponent(ROID id, ActiveComponents& ac)
		{
			ac.hasWaterComponent = true;
			return waterComponentTable->AddNew(id);
		}

		void Deprecate(ROID id, const ActiveComponents& ac) override
		{
			if (ac.hasFireComponent)
			{
				fireComponentTable->Deprecate(id);
			}
			if (ac.hasWaterComponent)
			{
				waterComponentTable->Deprecate(id);
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
		uint32 activeIdIndex = -1;	// Specifies which activeId slot references this object
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
			components(factories)
		{
			static_assert(sizeof ROID == sizeof uint64);

			uint64 maxTableSize = max(128ULL, maxTableSizeInBytes / (sizeof RCObject + sizeof ROID + sizeof ROID_TABLE_INDEX));

			this->maxTableEntries = maxTableSize > 4000'000ULL ? 4000'000 : (uint32) maxTableSize;

			freeIds.resize(this->maxTableEntries);

			activeIds.reserve(maxTableEntries);

			for (uint32 i = 1; i < maxTableEntries; i++)
			{
				ROID roid;
				roid.index = i;
				roid.salt = 1;
				freeIds.push_back(roid);
			}
		}

		size_t ActiveRoidCount() const override
		{
			return activeIds.size();
		}

		Ref<IFireComponent> AddFireComponent(ROID id)
		{
			if (id.index > 0 && id.index < maxTableEntries)
			{
				RCObject& object = handleTable[id.index];
				if (id.salt == object.salt)
				{
					return components.AddFireComponent(id, object.ac);
				}
			}

			return Ref<IFireComponent>();
		}
		Ref<IWaterComponent> AddWaterComponent(ROID id)
		{
			if (id.index > 0 && id.index < maxTableEntries)
			{
				RCObject& object = handleTable[id.index];
				if (id.salt == object.salt)
				{
					return components.AddWaterComponent(id, object.ac);
				}
			}

			return Ref<IWaterComponent>();
		}

		IComponentTables& Components()
		{
			return components;
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

				object.activeIdIndex = -1;

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

		bool DeprecateFireComponent(ROID id)
		{
			if (id.index > 0 && id.index < maxTableEntries)
			{
				RCObject& object = handleTable[id.index];
				if (id.salt == object.salt)
				{
					if (object.ac.hasFireComponent)
					{
						components.GetFireComponentTable().Deprecate(id);
						object.ac.hasFireComponent = false;
						return true;
					}
				}
			}

			return false;
		}
		bool DeprecateWaterComponent(ROID id)
		{
			if (id.index > 0 && id.index < maxTableEntries)
			{
				RCObject& object = handleTable[id.index];
				if (id.salt == object.salt)
				{
					if (object.ac.hasWaterComponent)
					{
						components.GetWaterComponentTable().Deprecate(id);
						object.ac.hasWaterComponent = false;
						return true;
					}
				}
			}

			return false;
		}

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
					if (cb.OnROID(id) == IROIDCallback::BREAK)
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

		Ref<IFireComponent> GetFireComponent(ROID id)
		{
			if (id.index > 0 && id.index < maxTableEntries)
			{
				RCObject& object = handleTable[id.index];
				if (id.salt == object.salt)
				{
					if (object.ac.hasFireComponent)
					{
						auto& table = components.GetFireComponentTable();
						return table.Find(id);
					}
				}
			}

			return Ref<IFireComponent>();
		}

		Ref<IWaterComponent> GetWaterComponent(ROID id)
		{
			if (id.index > 0 && id.index < maxTableEntries)
			{
				RCObject& object = handleTable[id.index];
				if (id.salt == object.salt)
				{
					if (object.ac.hasWaterComponent)
					{
						auto& table = components.GetWaterComponentTable();
						return table.Find(id);
					}
				}
			}

			return Ref<IWaterComponent>();
		}

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

namespace Rococo::Components::Sys::Factories
{
	IRCObjectTableSupervisor* Create_RCO_EntityComponentSystem(ComponentFactories& factories, uint64 maxSizeInBytes)
	{
		return new RCObjectTable(factories, maxSizeInBytes);
	}
}



