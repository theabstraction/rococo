#include "rococo.mplat.h"
#include "mplat.components.decl.h"
#include "rococo.animation.h"
#include <rococo.strings.h>
#include <new>

using namespace Rococo;
using namespace Rococo::Entities;
using namespace Rococo::Components;

struct AnimationComponent : IAnimationComponent
{
    AutoFree<IAnimation> animation;
    AnimationComponent()
    {
        animation = Entities::CreateAnimation();
    }

    IAnimation& GetAnimation()
    {
        return *animation;
    }
};

struct BodyComponent : IBodyComponent
{
    ID_SYS_MESH mesh;
    Matrix4x4 model;
    ROID parent;
    Vec3 scale{ 1.0f, 1.0f, 1.0f };

    cr_m4x4 Model() const override
    {
        return model;
    }

    ROID Parent() const override
    {
        return parent;
    }

    Vec3 Scale() const override
    {
        return scale;
    }

    void SetModel(cr_m4x4 model) override
    {
        this->model = model;
    }

    void SetParent(ROID parentId) override
    {
        this->parent = parentId;
    }

    void SetScale(cr_vec3 scale) override
    {
        this->scale = scale;
    }

    ID_SYS_MESH Mesh() const override
    {
        return mesh;
    }

    void SetMesh(ID_SYS_MESH id) override
    {
        mesh = id;
    }
};

struct SkeletonComponent : ISkeletonComponent
{
    ISkeletons& skeletons;
    HString skeletonName;
    FPSAngles fpsOrientation {0,0,0};
    ID_SKELETON skeletonId;

    SkeletonComponent(ISkeletons& _skeletons): skeletons(_skeletons)
    {

    }

    Entities::ISkeleton* Skeleton() override
    {
        ISkeleton* skeleton;
        if (skeletons.TryGet(skeletonId, &skeleton))
        {
            return skeleton;
        }
        else
        {
            return nullptr;
        }
    }

    void SetSkeleton(cstr skeletonName) override
    {
        this->skeletonName = skeletonName;
    }

    fstring SkeletonName() const override
    {
        return skeletonName.to_fstring();
    }

    void SetFPSOrientation(const FPSAngles& orientation) override
    {
        fpsOrientation = orientation;
    }

    const FPSAngles& FPSOrientation() const override
    {
        return fpsOrientation;
    }
};

struct ParticleSystemComponent : IParticleSystemComponent
{

};

struct RigsComponent : IRigsComponent
{

};

namespace Rococo::Components
{
    IComponentFactory<IAnimationComponent>* CreateAnimationFactory()
    {
        return new DefaultFactory<IAnimationComponent, AnimationComponent>();
    }

    IComponentFactory<IBodyComponent>* CreateBodyFactory()
    {
        return new DefaultFactory<IBodyComponent,BodyComponent>();
    }

    IComponentFactory<ISkeletonComponent>* CreateSkeletonFactory(Entities::ISkeletons& skeletons)
    {
        return new FactoryWithOneArg<ISkeletonComponent, SkeletonComponent, ISkeletons>(skeletons);
    }

    IComponentFactory<IParticleSystemComponent>* CreateParticleSystemFactory()
    {
        return new DefaultFactory<IParticleSystemComponent, ParticleSystemComponent>();
    }

    IComponentFactory<IRigsComponent>* CreateRigsFactory()
    {
        return new DefaultFactory<IRigsComponent, RigsComponent>();
    }
}