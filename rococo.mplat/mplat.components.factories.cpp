#include "rococo.mplat.h"
#include "mplat.components.decl.h"
#include <rococo.strings.h>
#include <new>

using namespace Rococo;
using namespace Rococo::Components;

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
    HString skeletonName;

    void SetSkeleton(cstr skeletonName) override
    {
        this->skeletonName = skeletonName;
    }

    fstring GetSkeletonName() const override
    {
        return skeletonName.to_fstring();
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
    IComponentFactory<IBodyComponent>* CreateBodyFactory()
    {
        return new DefaultFactory<IBodyComponent,BodyComponent>();
    }

    IComponentFactory<ISkeletonComponent>* CreateSkeletonFactory()
    {
        return new DefaultFactory<ISkeletonComponent, SkeletonComponent>();
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