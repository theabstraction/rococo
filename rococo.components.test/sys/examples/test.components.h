#pragma once

// Generated at: Feb 10 2022 22:19 UTC
// Based on the template file: C:\work\rococo\rococo.cpp_master\component.template.h
#include <rococo.types.h>
#include <rococo.component.entities.h>
#include "components.h"

namespace Rococo::Components::Sys
{
	using namespace Rococo::Components;

    template<class T>
	ROCOCOAPI IComponentTable
	{
		virtual void Free() = 0;
		virtual Ref<T> AddNew(EntityIndex id) = 0;
		[[nodiscard]] virtual Ref<T> Find(EntityIndex id) = 0;
		virtual void Deprecate(EntityIndex id) = 0;
		virtual void Flush() = 0;
	};

    struct ComponentFactories
    {
        IFireComponentFactory& fireComponentFactory;
        IWaterComponentFactory& waterComponentFactory;
    };

    struct ActiveComponents
    {
        bool hasFireComponent : 1;
        bool hasWaterComponent : 1;
    };

    // Rococo component object
    ROCOCOAPI IRCObject
    {
        virtual EntityIndex Index() const = 0;
        virtual ActiveComponents GetActiveComponents() const = 0;
    };

    ROCOCOAPI IRCObjectTable
    {
        virtual const IRCObject* FindRaw(EntityIndex index) const = 0;
    };

    ROCOCOAPI IRCObjectTableSupervisor: IRCObjectTable
    {
        virtual void Free() = 0;
    };

    ROCOCOAPI IComponentTables
    {
        virtual Ref<IFireComponent> AddFireComponent(EntityIndex index, ActiveComponents & ac) = 0;
        virtual Ref<IWaterComponent> AddWaterComponent(EntityIndex index, ActiveComponents & ac) = 0;

        virtual void Deprecate(EntityIndex index, const ActiveComponents& ac) = 0;

        [[nodiscard]] virtual IComponentTable<IFireComponent>& GetFireComponentTable() = 0;
        [[nodiscard]] virtual IComponentTable<IWaterComponent>& GetWaterComponentTable() = 0;
    };

    ROCOCOAPI IComponentTablesSupervisor : IComponentTables
    {
        virtual void Free() = 0;
    };

    namespace Factories
    {
        [[nodiscard]] IComponentTablesSupervisor* CreateComponentTables(ComponentFactories& factories);
    }
} // Rococo::Components::Sys

