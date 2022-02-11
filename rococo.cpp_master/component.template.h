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
		virtual Ref<T> AddNew(ROID id) = 0;
		[[nodiscard]] virtual Ref<T> Find(ROID id) = 0;
		virtual void Deprecate(ROID id) = 0;
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
        virtual Ref<IComponentInterface> AddComponentVariable(ROID index, ActiveComponents & ac) = 0;
// #END_INSTANCED#

        virtual void Deprecate(ROID index, const ActiveComponents& ac) = 0;

// #BEGIN_INSTANCED#
        [[nodiscard]] virtual IComponentTable<IComponentInterface>& GetComponentVariableTable() = 0;
// #END_INSTANCED#
    };

    struct IROIDCallback
    {
        enum EControlLogic { CONTINUE, BREAK };
        virtual EControlLogic OnROID(ROID id) = 0;
    };

    ROCOCOAPI IRCObjectTable
    {
        virtual size_t ActiveRoidCount() const = 0;
// #BEGIN_INSTANCED#
        virtual Ref<IComponentInterface> AddComponentVariable(ROID id) = 0;
// #END_INSTANCED#
        virtual IComponentTables& Components() = 0;
        virtual bool Deprecate(ROID roid) = 0;
        virtual void DeprecateAll() = 0;
// #BEGIN_INSTANCED#
        virtual bool DeprecateComponentVariable(ROID id) = 0;
// #END_INSTANCED#
        virtual void Enumerate(IROIDCallback& cb) = 0;
        virtual ActiveComponents GetActiveComponents(ROID id) const = 0;
// #BEGIN_INSTANCED#
        virtual Ref<IComponentInterface> GetComponentVariable(ROID id) = 0;
// #END_INSTANCED#
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
