#pragma once
#include <rococo.component.entities.h>

#ifndef ROCOCO_ECS_API 
# define ROCOCO_ECS_API ROCOCO_API_IMPORT
#endif

namespace Rococo
{
    struct IECS;
}

namespace Rococo::Components
{
    template<class T> ROCOCO_INTERFACE IComponentCallback
    {
        virtual EFlowLogic OnComponent(ROID id, T& item) = 0;
    };

    ROCOCO_INTERFACE IComponentBase
    {

    };

    ROCOCO_INTERFACE IComponentTable
    {
        virtual IECS & ECS() = 0;
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
        virtual IComponentBase* Create(ROID roid) = 0;
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
    ROCOCO_ECS_API IRoidMap* CreateRoidMap(cstr friendlyName, size_t reserveRows);

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
    using namespace Rococo::Components;

    struct IECS;

    ROCOCO_INTERFACE IECS_ROID_LockedSection
    {
        virtual void OnLock(ROID roid, IECS& ecs) = 0;
    };

	ROCOCO_INTERFACE IECS
	{
        // Returns the number of ROIDS in use. 
        [[nodiscard]] virtual size_t ActiveRoidCount() const = 0;

        // Called periodically to remove deprecated and unreferenced entities and components
        virtual void CollectGarbage() = 0;

        // Mark a ROID as deprecated, existant Ref<IComponentInterfaces> will be valid, but, when all new references can no longer be accessed.
        virtual bool Deprecate(ROID roid) = 0;

        // Mark all ROIDs as deprecated. Best followed up by a CollectGarbage.
        virtual void DeprecateAll() = 0;

        virtual void Enumerate(IROIDCallback& cb) = 0;

        // Returns true if a ROID is in the system and not deprecated
        [[nodiscard]] virtual bool IsActive(ROID id) const = 0;

        // Returns the maximum number of ROIDS that can be managed by this system.
        [[nodiscard]] virtual uint32 MaxTableEntries() const = 0;

        // Creates a new ROID and returns the result
        [[nodiscard]] virtual ROID NewROID() = 0;

        virtual bool TryLockedOperation(ROID roid, IECS_ROID_LockedSection& section) = 0;
	};

    ROCOCO_INTERFACE IECSSupervisor : IECS
    {
        // Add a reference to a given component table. The reference must be valid for the lifetime of the ECS system. The ECS system must therefore be destroyed before all linked tables
        virtual void LinkComponentTable(IComponentTable& table) = 0;
		virtual void Free() = 0;
	};

    // The consumer of the ECS system provides the implementation for the error handler. Typically it should throw an exception and emit the text data provided
    ROCOCO_INTERFACE IECSErrorHandler
    {
        enum class ECS_ErrorCause
        {
            GC_Locked_Enumeration,
            OutOfRoids,
            LinkComponentTable_Duplicate
        };

        virtual void OnError(cstr functionName, int lineNumber, cstr message, bool expectedToThrow, ECS_ErrorCause cause) = 0;
    };

	ROCOCO_ECS_API IECSSupervisor* CreateECS(IECSErrorHandler& errorHandler, uint64 maxTableSizeInBytes = 2_gigabytes);
}
