#pragma once

#include <rococo.types.h>
#include <rococo.mplat.h>

namespace Rococo::Components
{
    ROCOCOAPI IEntity
    {
        virtual cr_m4x4 Model() const = 0;
        virtual ROID Parent() const = 0;
        virtual Vec3 Scale() const = 0;
        virtual void SetModel(cr_m4x4 model) = 0;
        virtual void SetScale(cr_vec3 scale) = 0;
    };

    ROCOCOAPI IBodyComponent : IEntity
    {
        virtual ID_SYS_MESH Mesh() const = 0;
        virtual void SetMesh(ID_SYS_MESH meshId) = 0;
    };

    ROCOCOAPI ISkeletonComponent
    {
        virtual void SetSkeleton(cstr skeletonName) = 0;
        virtual fstring GetSkeletonName() const = 0;
    };

    ROCOCOAPI IRigsComponent
    {

    };

    ROCOCOAPI IParticleSystemComponent
    {

    };

    template<class ICOMPONENT>
    ROCOCOAPI IComponentFactory
    {
        virtual ICOMPONENT * ConstructInPlace(void* pMemory) = 0;
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

    IComponentFactory<IBodyComponent>* CreateBodyFactory();
    IComponentFactory<ISkeletonComponent>* CreateSkeletonFactory();
    IComponentFactory<IParticleSystemComponent>* CreateParticleSystemFactory();
    IComponentFactory<IRigsComponent>* CreateRigsFactory();
}