#pragma once

// Generated at: Feb 07 2022 T UTC
// Based on the template file: C:\work\rococo\rococo.cpp_master\component.template.h

#include <rococo.api.h>
#include <list>
#include <unordered_map>
#include "rococo.component.entities.h"
#include "components.h"


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

namespace Rococo::Components::Sys::Factories
{
	IFireComponentTable* NewComponentInterfaceTable(IFireComponentFactory& factory);
}

namespace Rococo::Components::Sys
{
	using namespace Rococo::Components;

	ROCOCOAPI IWaterComponentTable
	{
		virtual void Free() = 0;
		virtual IWaterComponent* AddNew(EntityIndex id) = 0;
		virtual IWaterComponent* Find(EntityIndex id) = 0;
		virtual void Deprecate(EntityIndex id) = 0;
		virtual void Flush() = 0;
	};
}

namespace Rococo::Components::Sys::Factories
{
	IWaterComponentTable* NewComponentInterfaceTable(IWaterComponentFactory& factory);
}

namespace Rococo::Components::Sys
{
	using namespace Rococo::Components;

	struct ComponentFactories
	{
		IFireComponentFactory& fireComponentfactory;
		IWaterComponentFactory& waterComponentfactory;
	};

	struct ActiveComponents
	{
		bool hasFireComponent: 1;
		bool hasWaterComponent: 1;
	};

	ROCOCOAPI IComponentTables
	{
		virtual IFireComponent* AddFireComponent(EntityIndex index, ActiveComponents& ac) = 0;
		virtual IWaterComponent* AddWaterComponent(EntityIndex index, ActiveComponents& ac) = 0;

		virtual void Deprecate(EntityIndex index, const ActiveComponents& ac) = 0;

		virtual IFireComponentTable& GetFireComponentTable() = 0;
		virtual IWaterComponentTable& GetWaterComponentTable() = 0;
	};

	ROCOCOAPI IComponentTablesSupervisor: IComponentTables
	{
		virtual void Free() = 0;
	};

	IComponentTablesSupervisor* CreateComponentTables(ComponentFactories& factories);
}
