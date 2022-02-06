#pragma once

#include <rococo.api.h>
#include <list>
#include <unordered_map>
#include "rococo.component.entities.h"
#include "$declarations.h"

// Template-MetaData

namespace Rococo::Components::Sys
{
	using namespace Rococo::Components;

	ROCOCOAPI IComponentInterfaceTable
	{
		virtual void Free() = 0;
		virtual IComponentInterface* AddNew(EntityIndex id) = 0;
		virtual IComponentInterface* Find(EntityIndex id) = 0;
		virtual void Deprecate(EntityIndex id) = 0;
		virtual void Flush() = 0;
	};
}

/*
	Requirements of a component container.
	1. Fast allocation and fast release of individual elements
	2. Fast enumeration
	3. Constant time lookup by ENTITY_ID
	4. Constant time insertion and removal
	5. Dynamic expansion of container
 */

namespace ANON
{
	using namespace Rococo;
	using namespace Rococo::Components::Sys;

	struct ComponentInterfaceTable : IComponentInterfaceTable
	{
		IComponentInterfaceFactory& componentFactory;
		std::unordered_map<EntityIndex, IComponentInterface*, EntityIndexHasher, EntityIndexComparer> rows;
		std::list<EntityIndex> deprecatedList;
		AutoFree<IFreeListAllocatorSupervisor> componentAllocator;

		ComponentInterfaceTable(IComponentInterfaceFactory& factory): componentFactory(factory), rows(1024)
		{
			componentAllocator = CreateFreeListAllocator(factory.SizeOfConstructedObject());
		}

		void Free() override
		{
			delete this;
		}

		IComponentInterface* AddNew(EntityIndex index) override
		{
			std::pair<EntityIndex, IComponentInterface*> nullItem(index, nullptr);
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
				i->second = component;
				return component;
			}
			catch (...)
			{
				rows.erase(i);
				throw;
			}
		}

		IComponentInterface* Find(EntityIndex index) override
		{
			auto i = rows.find(index);
			return i != rows.end() ? i->second : nullptr;
		}

		void Deprecate(EntityIndex index) override
		{
			auto i = rows.find(index);
			if (i != rows.end())
			{
				auto* component = i->second;
				if (component->Deprecate())
				{
					deprecatedList.push_back(index);
				}
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
					auto* component = it->second;
					if (component->IsReadyToDelete())
					{
						i = deprecatedList.erase(i);
						component->Free();
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
}

namespace Rococo::Components::Sys::Factories
{
	IComponentInterfaceTable* NewComponentInterfaceTable(IComponentInterfaceFactory& factory)
	{
		return new ANON::ComponentInterfaceTable(factory);
	}
}