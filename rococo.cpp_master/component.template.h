#include <rococo.types.h>
#include <rococo.component.entities.h>
#include "DeclarationsInclude"

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
// #BEGIN_INSTANCED#
        IComponentInterfaceFactory& componentVariableFactory;
// #END_INSTANCED#
    };

    struct ActiveComponents
    {
// #BEGIN_INSTANCED#
        bool hasComponentVariable : 1;
// #END_INSTANCED#
    };

    ROCOCOAPI IComponentTables
    {
// #BEGIN_INSTANCED#
        virtual Ref<IComponentInterface> AddComponentVariable(EntityIndex index, ActiveComponents & ac) = 0;
// #END_INSTANCED#

        virtual void Deprecate(EntityIndex index, const ActiveComponents& ac) = 0;

// #BEGIN_INSTANCED#
        [[nodiscard]] virtual IComponentTable<IComponentInterface>& GetComponentVariableTable() = 0;
// #END_INSTANCED#
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
