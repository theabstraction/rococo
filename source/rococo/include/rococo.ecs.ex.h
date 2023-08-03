#pragma once
#include <rococo.ecs.h>
#include <rococo.component.entities.h>

namespace Rococo::Components
{
    template<class T> ROCOCO_INTERFACE IComponentCallback
    {
        virtual EFlowLogic OnComponent(ROID id, T& item) = 0;
    };

    ROCOCO_INTERFACE IComponentTable
    {
        virtual IECS & ECS() = 0;
    };

    ROCOCO_INTERFACE IComponentTableSupervisor : IComponentTable
    {
        virtual void BindTableIndex(uint32 tableIndex) = 0;
        virtual void Deprecate(ROID roid) = 0;
        virtual void OnNotifyThatTheECSHasDeprecatedARoid(ROID roid) = 0;
    };

    struct RoidEntry
    {
        ROID id;
        IComponentBase* base;
    };

    template<class T>
    struct TRoidEntry
    {
        ROID id;
        T* component;
    };

    ROCOCO_INTERFACE IComponentBaseFactory
    {
        // Generates a new component for the given roid. If the method returns nullptr creation failed. The implementation may also throw an exception
        virtual IComponentBase * Create(ROID roid) = 0;
    };

    ROCOCO_INTERFACE IRoidMap
    {
        virtual void Free() = 0;
        virtual void Delete(ROID id, IEventCallback<IComponentBase*>& onDelete) = 0;
        virtual IComponentBase* Find(ROID id) const = 0;
        virtual void ForEachComponent(IComponentCallback<IComponentBase>& cb) = 0;

        // Copy the first [nElementsInOutput] to the output buffer (if the buffer is not null). Returns the length of the map
        // To copy everything call 'length' = ToVector(nullptr, 0) then allocate a buffer of that length and then call ToVector(nullptr, 'length')
        virtual size_t ToVector(RoidEntry* roidOutput, size_t nElementsInOutput) = 0;

        // Allocate mapping for a ROID, using the factory to create an associated component if successfully reserved. If the factory returns nullptr mapping is revoked.
        // If the factory throws an exception mapping is revoked and the exception propagated to the caller.
        // If the ROID already exists in the map an exception is thrown and the factory methods are not invoked
        virtual void Insert(ROID id, IComponentBaseFactory& factory) = 0;
    };

    // Create a ROID vs IComponentBase* map. The map does not invoke interface deletion, this is task for the caller of the API
    // The friendly name must not be null and specifies a help string emitted during exceptions
    ROCOCO_ECS_API IRoidMap* CreateRoidMap(const char* const friendlyName, size_t reserveRows);

    template<class ICOMPONENT>
    ROCOCO_INTERFACE IComponentFactory
    {
        virtual ICOMPONENT* ConstructInPlace(void* pMemory) = 0;
        virtual void Destruct(ICOMPONENT* pInstance) = 0;
        virtual size_t SizeOfConstructedObject() const = 0;
        virtual void Free() = 0;
    };

    // Provide a lightweight implementation of an IComponentFactory<T> for use with components that are default constructed.
    template<class INTERFACE, class CLASSNAME>
    struct DefaultFactory : IComponentFactory<INTERFACE>
    {
        INTERFACE* ConstructInPlace(void* pMemory) override
        {
            // Requires #include <new>. 
            // This was not added to the header as it adds thousands of lines to the compile unit for everything that includes this file
            return new (pMemory) CLASSNAME();
        }

        void Destruct(INTERFACE* pInstance) override
        {
            CLASSNAME* bc = static_cast<CLASSNAME*>(pInstance);
            bc->~CLASSNAME();
        }

        size_t SizeOfConstructedObject() const override
        {
            return sizeof CLASSNAME;
        }

        void Free() override
        {
            delete this;
        }
    };

    template<class INTERFACE, class CLASSNAME, class ARG>
    struct FactoryWithOneArg : IComponentFactory<INTERFACE>
    {
        ARG& arg;

        FactoryWithOneArg(ARG& _arg) : arg(_arg) {}

        INTERFACE* ConstructInPlace(void* pMemory) override
        {
            return new (pMemory) CLASSNAME(arg);
        }

        void Destruct(INTERFACE* pInstance) override
        {
            CLASSNAME* bc = static_cast<CLASSNAME*>(pInstance);
            bc->~CLASSNAME();
        }

        size_t SizeOfConstructedObject() const override
        {
            return sizeof CLASSNAME;
        }

        void Free() override
        {
            delete this;
        }
    };
}

namespace Rococo
{
    ROCOCO_INTERFACE IECSSupervisor : IECS
    {
        // Add a reference to a given component table. The reference must be valid for the lifetime of the ECS system. The ECS system must therefore be destroyed before all linked tables
        // The [return] value is the [tableIndex] of the given table that is presented to the ECS in some component callbacks
        virtual void LinkComponentTable(IComponentTableSupervisor & table) = 0;

        // Tells the ECS that a ROID has been deprecated by a table. The ECS will pass on the event to all observers, except the source of the deprecation.
        virtual void OnNotifyAComponentHasDeprecatedARoid(ROID roid, uint32 sourceTableIndex) = 0;
        virtual void OnNotifyComponentAttachedToROID(ROID roid, uint32 sourceTableIndex) = 0;
        virtual void Free() = 0;
    };

    ROCOCO_ECS_API IECSSupervisor* CreateECS(uint64 maxTableSizeInBytes = 256_megabytes);

    ROCOCO_INTERFACE IComponentTableManagerBase
    {
        virtual void Free() = 0;
    };
}

#include <rococo.functional.h>

#define LINK_NAME(y, z) LinkToECS_##y##z

// Publishes the component Add, Get and Enumerate API, as well as methods for linking the component table to the ECS system.
#define DECLARE_SINGLETON_METHODS(COMPONENT_API,COMPONENT)													\
namespace Rococo::Components::API::For##COMPONENT													        \
{																									        \
	COMPONENT_API Ref<COMPONENT> Add(ROID id);														        \
	COMPONENT_API Ref<COMPONENT> Get(ROID id);														        \
    COMPONENT_API void ForEach(Function<EFlowLogic(ROID roid, COMPONENT&)> functor);                        \
}                                                                                                           \
namespace Rococo::Components::ECS			        														\
{																									        \
	COMPONENT_API void ReleaseTablesFor##COMPONENT();														\
    COMPONENT_API void LINK_NAME(COMPONENT, Table)(IECSSupervisor& ecs);                                    \
}																								            

// Publishes the component Add, Get and Enumerate API, as well as methods for linking the component table to the ECS system.
// The link arg allows the caller to supply the component with interfaces and any anything else it needs to do its job.
// For multiple arguments create a struct and add the arguments as fields
#define DECLARE_SINGLETON_METHODS_WITH_LINK_ARG(COMPONENT_API,COMPONENT, LINK_ARG)							\
namespace Rococo::Components::API::For##COMPONENT													        \
{																									        \
	COMPONENT_API Ref<COMPONENT> Add(ROID id);														        \
	COMPONENT_API Ref<COMPONENT> Get(ROID id);														        \
    COMPONENT_API void ForEach(Function<EFlowLogic(ROID roid, COMPONENT&)> functor);                        \
}                                                                                                           \
namespace Rococo::Components::ECS			        														\
{																									        \
	COMPONENT_API void ReleaseTablesFor##COMPONENT();														\
    COMPONENT_API void LINK_NAME(COMPONENT, Table)(IECSSupervisor& ecs, LINK_ARG& arg);                     \
}																								            