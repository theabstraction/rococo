#pragma once

// Generated at: Feb 11 2022 13:51 UTC
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
		virtual Ref<T> AddNew(ROID id) = 0;
		[[nodiscard]] virtual Ref<T> Find(ROID id) = 0;
		virtual void Deprecate(ROID id) = 0;
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

    ROCOCOAPI IComponentTables
    {
        virtual Ref<IFireComponent> AddFireComponent(ROID index, ActiveComponents & ac) = 0;
        virtual Ref<IWaterComponent> AddWaterComponent(ROID index, ActiveComponents & ac) = 0;

        virtual void Deprecate(ROID index, const ActiveComponents& ac) = 0;

        [[nodiscard]] virtual IComponentTable<IFireComponent>& GetFireComponentTable() = 0;
        [[nodiscard]] virtual IComponentTable<IWaterComponent>& GetWaterComponentTable() = 0;
    };

    struct IROIDCallback
    {
        enum EControlLogic { CONTINUE, BREAK };
        virtual EControlLogic OnROID(ROID id) = 0;
    };

    ROCOCOAPI IRCObjectTable
    {
        virtual size_t ActiveRoidCount() const = 0;
        virtual Ref<IFireComponent> AddFireComponent(ROID id) = 0;
        virtual Ref<IWaterComponent> AddWaterComponent(ROID id) = 0;
        virtual IComponentTables& Components() = 0;
        virtual bool Deprecate(ROID roid) = 0;
        virtual void DeprecateAll() = 0;
        virtual bool DeprecateFireComponent(ROID id) = 0;
        virtual bool DeprecateWaterComponent(ROID id) = 0;
        virtual void Enumerate(IROIDCallback& cb) = 0;
        virtual ActiveComponents GetActiveComponents(ROID id) const = 0;
        virtual Ref<IFireComponent> GetFireComponent(ROID id) = 0;
        virtual Ref<IWaterComponent> GetWaterComponent(ROID id) = 0;
        virtual bool IsActive(ROID id) const = 0;
        virtual uint32 MaxTableEntries() const = 0;
        virtual ROID NewROID() = 0;
    };

    ROCOCOAPI IRCObjectTableSupervisor : IRCObjectTable
    {
        virtual void Free() = 0;
    };

    namespace Factories
    {
        [[nodiscard]] IRCObjectTableSupervisor* Create_RCO_EntityComponentSystem(ComponentFactories& factories, uint64 maxSizeInBytes = 2 * 1024 * 1024 * 1024ULL);
    }
} // Rococo::Components::Sys

