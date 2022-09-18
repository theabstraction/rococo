#pragma once

namespace Rococo::Entities
{
    struct IAnimation;
    struct ISkeleton;
    struct ISkeletons;
}

namespace Rococo::Components
{
    ROCOCO_INTERFACE IEntity
    {
        virtual cr_m4x4 Model() const = 0;
        virtual ROID Parent() const = 0;
        virtual Vec3 Scale() const = 0;
        virtual void SetModel(cr_m4x4 model) = 0;
        virtual void SetParent(ROID parent) = 0;
        virtual void SetScale(cr_vec3 scale) = 0;
    };

    ROCOCO_INTERFACE IBodyComponent : IEntity
    {
        virtual ID_SYS_MESH Mesh() const = 0;
        virtual void SetMesh(ID_SYS_MESH meshId) = 0;
    };

    ROCOCO_INTERFACE ISkeletonComponent
    {
        virtual Entities::ISkeleton* Skeleton() = 0;
        virtual void SetSkeleton(cstr skeletonName) = 0;
        virtual fstring SkeletonName() const = 0;
        virtual void SetFPSOrientation(const FPSAngles& orientation) = 0;
        virtual const FPSAngles& FPSOrientation() const = 0;
    };

    ROCOCO_INTERFACE IRigsComponent
    {

    };

    ROCOCO_INTERFACE IParticleSystemComponent
    {

    };

    ROCOCO_INTERFACE IAnimationComponent
    {
        // Temporary reference, do not cache
        virtual Rococo::Entities::IAnimation& GetAnimation() = 0;
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

    IComponentFactory<IAnimationComponent>* CreateAnimationFactory();
    IComponentFactory<IBodyComponent>* CreateBodyFactory();
    IComponentFactory<ISkeletonComponent>* CreateSkeletonFactory(Entities::ISkeletons& skeletons);
    IComponentFactory<IParticleSystemComponent>* CreateParticleSystemFactory();
    IComponentFactory<IRigsComponent>* CreateRigsFactory();
    IComponentFactory<IAnimationComponent> CreateAnimationComponent();
}