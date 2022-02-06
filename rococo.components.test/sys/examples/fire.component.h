#pragma once

#include <rococo.api.h>
#include <list>
#include <unordered_map>
#include "rococo.component.entities.h"
#include "components.h"

// Meta data:
// Generated at: Feb 6 2022 18.39.22 UTC
// Based on the template file: C:\work\rococo\rococo.cpp_master\component.template.h


namespace Rococo::Components::Sys
{
	using namespace Rococo::Components;

	ROCOCOAPI IFireComponentTable
	{
		virtual void Free() = 0;
		virtual IFireComponent* AddNew(EntityIndex id) = 0;
		virtual IFireComponent* Find(EntityIndex id) = 0;
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

	struct ComponentInterfaceTable : IFireComponentTable
	{
		IFireComponentFactory& componentFactory;
		std::unordered_map<EntityIndex, IFireComponent*, EntityIndexHasher, EntityIndexComparer> rows;
		std::list<EntityIndex> deprecatedList;
		AutoFree<IFreeListAllocatorSupervisor> componentAllocator;

		ComponentInterfaceTable(IFireComponentFactory& factory): componentFactory(factory), rows(1024)
		{
			componentAllocator = CreateFreeListAllocator(factory.SizeOfConstructedObject());
		}

		void Free() override
		{
			delete this;
		}

		IFireComponent* AddNew(EntityIndex index) override
		{
			std::pair<EntityIndex, IFireComponent*> nullItem(index, nullptr);
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
				i->second = component;
				return component;
			}
			catch (...)
			{
				rows.erase(i);
				throw;
			}
		}

		IFireComponent* Find(EntityIndex index) override
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
	IFireComponentTable* NewComponentInterfaceTable(IFireComponentFactory& factory)
	{
		return new ANON::ComponentInterfaceTable(factory);
	}
}