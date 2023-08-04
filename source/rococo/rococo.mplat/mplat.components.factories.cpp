#include "rococo.mplat.h"
#include "mplat.components.decl.h"
#include "rococo.animation.h"
#include <rococo.strings.h>
#include <new>

using namespace Rococo;
using namespace Rococo::Entities;
using namespace Rococo::Components;
using namespace Rococo::Strings;

struct ParticleSystemComponent : IParticleSystemComponent
{
    ParticleSystemComponent(InstanceInfo& info)
    {
        UNUSED(info);
    }
};

struct RigsComponent : IRigsComponent
{
    RigsComponent(InstanceInfo& info)
    {
        UNUSED(info);
    }
};

namespace Rococo::Components
{
    IComponentFactory<IParticleSystemComponent>* CreateParticleSystemFactory()
    {
        return new DefaultFactory<IParticleSystemComponent, ParticleSystemComponent>();
    }

    IComponentFactory<IRigsComponent>* CreateRigsFactory()
    {
        return new DefaultFactory<IRigsComponent, RigsComponent>();
    }
}