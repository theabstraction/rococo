#pragma once

// Generated at: Aug 25 2022 20:10 UTC
// Based on the template file: C:\work\rococo\rococo.mplat\mplat.component.template.h
#include <rococo.types.h>
#include <rococo.component.entities.h>
#include "mplat.components.decl.h"
#include <rococo.functional.h>

namespace Rococo::Components
{
    struct ComponentFactories
    {
        IComponentFactory<IAnimationComponent>& animationComponentFactory;
        IComponentFactory<IBodyComponent>& bodyComponentFactory;
        IComponentFactory<ISkeletonComponent>& skeletonComponentFactory;
        IComponentFactory<IParticleSystemComponent>& particleSystemComponentFactory;
        IComponentFactory<IRigsComponent>& rigsComponentFactory;
    };

    struct ActiveComponents
    {
        bool hasAnimationComponent : 1;
        bool hasBodyComponent : 1;
        bool hasSkeletonComponent : 1;
        bool hasParticleSystemComponent : 1;
        bool hasRigsComponent : 1;
    };

    ROCOCOAPI IROIDCallback
    {
        virtual EFlowLogic OnROID(ROID id) = 0;
    };

    template<class T> ROCOCOAPI IComponentCallback
    {
        virtual EFlowLogic OnComponent(ROID id, T& item) = 0;
    };

    ROCOCOAPI IRCObjectTableBase
    {
        // Returns the number of ROIDS in use. 
        [[nodiscard]] virtual size_t ActiveRoidCount() const = 0;

        // Called periodically to remove deprecated and unreferenced entities and components
        virtual void CollectGarbage() = 0;

        // Mark a ROID as deprecated, existant Ref<IComponentInterfaces> will be valid, but, when all new references can no longer be accessed.
        virtual bool Deprecate(ROID roid) = 0;

        // Mark all ROIDs as deprecated. Best followed up by a CollectGarbage.
        virtual void DeprecateAll() = 0;

        // Invoke the cb.OnROID for every ROID
        virtual void Enumerate(IROIDCallback& cb) = 0;

        // Returns the component flags structure.
        [[nodiscard]] virtual ActiveComponents GetActiveComponents(ROID id) const = 0;

        // Returns true if a ROID is in the system and not deprecated
        [[nodiscard]] virtual bool IsActive(ROID id) const = 0;

        // Returns the maximum number of ROIDS that can be managed by this system.
        [[nodiscard]] virtual uint32 MaxTableEntries() const = 0;

        // Creates a new ROID and returns the result
        [[nodiscard]] virtual ROID NewROID() = 0;
    };

    ROCOCOAPI IRCObjectTable: IRCObjectTableBase
    {     

        [[nodiscard]] virtual Ref<IAnimationComponent> AddAnimationComponent(ROID id) = 0;

        // Marks the AnimationComponent as deprecated, when all outstanding refences are out of scope the object can be garbage collected
        virtual bool DeprecateAnimationComponent(ROID id) = 0;

        // Enumerate all AnimationComponent elements. The reference in the callback cb.OnComponent is valid only for the callback lifetime. 
        // While enumerating garbage collection is suspended and new items cannot be added.
        virtual void ForEachAnimationComponent(IComponentCallback<IAnimationComponent>& cb) = 0;

        // Enumerate all AnimationComponent elements. The reference in the callback cb.OnComponent is valid only for the callback lifetime. 
        // While enumerating garbage collection is suspended and new items cannot be added.
        virtual void ForEachAnimationComponent(Rococo::Function<EFlowLogic(ROID id, IAnimationComponent& component)> functor) = 0;

        // Attemp to get a reference to the component with a given ROID
        virtual Ref<IAnimationComponent> GetAnimationComponent(ROID id) = 0;

        // Populate an array of ROIDs and return the number appended. If roidOutput is null then the return value is the number of AnimationComponents in the table
        [[nodiscard]] virtual size_t GetAnimationComponentIDs(ROID* roidOutput, size_t nElementsInOutput) = 0;

        [[nodiscard]] virtual Ref<IBodyComponent> AddBodyComponent(ROID id) = 0;

        // Marks the BodyComponent as deprecated, when all outstanding refences are out of scope the object can be garbage collected
        virtual bool DeprecateBodyComponent(ROID id) = 0;

        // Enumerate all BodyComponent elements. The reference in the callback cb.OnComponent is valid only for the callback lifetime. 
        // While enumerating garbage collection is suspended and new items cannot be added.
        virtual void ForEachBodyComponent(IComponentCallback<IBodyComponent>& cb) = 0;

        // Enumerate all BodyComponent elements. The reference in the callback cb.OnComponent is valid only for the callback lifetime. 
        // While enumerating garbage collection is suspended and new items cannot be added.
        virtual void ForEachBodyComponent(Rococo::Function<EFlowLogic(ROID id, IBodyComponent& component)> functor) = 0;

        // Attemp to get a reference to the component with a given ROID
        virtual Ref<IBodyComponent> GetBodyComponent(ROID id) = 0;

        // Populate an array of ROIDs and return the number appended. If roidOutput is null then the return value is the number of BodyComponents in the table
        [[nodiscard]] virtual size_t GetBodyComponentIDs(ROID* roidOutput, size_t nElementsInOutput) = 0;

        [[nodiscard]] virtual Ref<ISkeletonComponent> AddSkeletonComponent(ROID id) = 0;

        // Marks the SkeletonComponent as deprecated, when all outstanding refences are out of scope the object can be garbage collected
        virtual bool DeprecateSkeletonComponent(ROID id) = 0;

        // Enumerate all SkeletonComponent elements. The reference in the callback cb.OnComponent is valid only for the callback lifetime. 
        // While enumerating garbage collection is suspended and new items cannot be added.
        virtual void ForEachSkeletonComponent(IComponentCallback<ISkeletonComponent>& cb) = 0;

        // Enumerate all SkeletonComponent elements. The reference in the callback cb.OnComponent is valid only for the callback lifetime. 
        // While enumerating garbage collection is suspended and new items cannot be added.
        virtual void ForEachSkeletonComponent(Rococo::Function<EFlowLogic(ROID id, ISkeletonComponent& component)> functor) = 0;

        // Attemp to get a reference to the component with a given ROID
        virtual Ref<ISkeletonComponent> GetSkeletonComponent(ROID id) = 0;

        // Populate an array of ROIDs and return the number appended. If roidOutput is null then the return value is the number of SkeletonComponents in the table
        [[nodiscard]] virtual size_t GetSkeletonComponentIDs(ROID* roidOutput, size_t nElementsInOutput) = 0;

        [[nodiscard]] virtual Ref<IParticleSystemComponent> AddParticleSystemComponent(ROID id) = 0;

        // Marks the ParticleSystemComponent as deprecated, when all outstanding refences are out of scope the object can be garbage collected
        virtual bool DeprecateParticleSystemComponent(ROID id) = 0;

        // Enumerate all ParticleSystemComponent elements. The reference in the callback cb.OnComponent is valid only for the callback lifetime. 
        // While enumerating garbage collection is suspended and new items cannot be added.
        virtual void ForEachParticleSystemComponent(IComponentCallback<IParticleSystemComponent>& cb) = 0;

        // Enumerate all ParticleSystemComponent elements. The reference in the callback cb.OnComponent is valid only for the callback lifetime. 
        // While enumerating garbage collection is suspended and new items cannot be added.
        virtual void ForEachParticleSystemComponent(Rococo::Function<EFlowLogic(ROID id, IParticleSystemComponent& component)> functor) = 0;

        // Attemp to get a reference to the component with a given ROID
        virtual Ref<IParticleSystemComponent> GetParticleSystemComponent(ROID id) = 0;

        // Populate an array of ROIDs and return the number appended. If roidOutput is null then the return value is the number of ParticleSystemComponents in the table
        [[nodiscard]] virtual size_t GetParticleSystemComponentIDs(ROID* roidOutput, size_t nElementsInOutput) = 0;

        [[nodiscard]] virtual Ref<IRigsComponent> AddRigsComponent(ROID id) = 0;

        // Marks the RigsComponent as deprecated, when all outstanding refences are out of scope the object can be garbage collected
        virtual bool DeprecateRigsComponent(ROID id) = 0;

        // Enumerate all RigsComponent elements. The reference in the callback cb.OnComponent is valid only for the callback lifetime. 
        // While enumerating garbage collection is suspended and new items cannot be added.
        virtual void ForEachRigsComponent(IComponentCallback<IRigsComponent>& cb) = 0;

        // Enumerate all RigsComponent elements. The reference in the callback cb.OnComponent is valid only for the callback lifetime. 
        // While enumerating garbage collection is suspended and new items cannot be added.
        virtual void ForEachRigsComponent(Rococo::Function<EFlowLogic(ROID id, IRigsComponent& component)> functor) = 0;

        // Attemp to get a reference to the component with a given ROID
        virtual Ref<IRigsComponent> GetRigsComponent(ROID id) = 0;

        // Populate an array of ROIDs and return the number appended. If roidOutput is null then the return value is the number of RigsComponents in the table
        [[nodiscard]] virtual size_t GetRigsComponentIDs(ROID* roidOutput, size_t nElementsInOutput) = 0;
    };

    ROCOCOAPI IRCObjectTableSupervisor : IRCObjectTable
    {
        virtual void Free() = 0;
    };

    namespace Factories
    {
        /*
        *   Returns an interface to a new RCO ECS system.
        *   The factories structure is generated by rococo.cpp_master acting upon component.template.cpp and component.template.h
        *   The maxSizeInBytes value gives the memory dedicated to the entity tables, not the associated components. 2GB is the default
        */
        [[nodiscard]] IRCObjectTableSupervisor* Create_RCO_EntityComponentSystem(ComponentFactories& factories, uint64 maxSizeInBytes = 2 * 1024 * 1024 * 1024ULL);
    }
} // Rococo::Components

