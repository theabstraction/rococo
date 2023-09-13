#define ROCOCO_COMPONENTS_SKELETON_API __declspec(dllexport)

#include <components/rococo.components.skeleton.h>
#include <rococo.maths.h>
#include <rococo.strings.h>
#include <rococo.animation.h>
#include <components/rococo.ecs.builder.inl>
#include <new>

namespace Rococo::Components
{
    using namespace Rococo::Entities;
    using namespace Rococo::Strings;

	struct SkeletonComponent : ISkeletonComponent
	{	
        ISkeletons& skeletons;
        HString skeletonName;
        FPSAngles fpsOrientation{ 0,0,0 };
        ID_SKELETON skeletonId;

        SkeletonComponent(ISkeletons& _skeletons, InstanceInfo&) : skeletons(_skeletons)
        {

        }

        void GetSkeletonName(Rococo::Strings::IStringPopulator& sb) const override
        {
            sb.Populate(skeletonName);
        }

        void SetSkeletonByName(const fstring& name) override
        {
            skeletonName = name;
        }

        ComponentTypeInfo TypeInfo() const override
        {
            return ComponentTypeInfo{ "SkeletonComponent" };
        };

        void Reflect(ComponentReflectionInfo& info) override
        {
            UNUSED(info);
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
                skeletonId = skeletons.GetByNameAndReturnId(skeletonName, &skeleton);
                return skeleton;
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
}

namespace Module::ForISkeletonComponent
{
    using namespace Rococo::Components;
    IComponentFactory<ISkeletonComponent>* CreateComponentFactory(ISkeletons& skeletons)
    {
        return new FactoryWithOneArg<ISkeletonComponent, SkeletonComponent, ISkeletons&>(skeletons);
    }
}

DEFINE_AND_EXPORT_SINGLETON_METHODS_WITH_LINKARG(ROCOCO_COMPONENTS_SKELETON_API, ISkeletonComponent, Entities::ISkeletons&)
