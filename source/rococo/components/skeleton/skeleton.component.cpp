#define ROCOCO_COMPONENTS_SKELETON_API __declspec(dllexport)

#include <components/rococo.components.skeleton.h>
#include <rococo.maths.h>
#include <rococo.strings.h>
#include <rococo.animation.h>
#include <rococo.ecs.builder.inl>
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

        SkeletonComponent(ISkeletons& _skeletons) : skeletons(_skeletons)
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
}

namespace Module::ForISkeletonComponent
{
    using namespace Rococo::Components;
    IComponentFactory<ISkeletonComponent>* CreateComponentFactory(ISkeletons& skeletons)
    {
        return new FactoryWithOneArg<ISkeletonComponent, SkeletonComponent, ISkeletons>(skeletons);
    }
}

DEFINE_AND_EXPORT_SINGLETON_METHODS_WITH_LINKARG(ROCOCO_COMPONENTS_SKELETON_API, ISkeletonComponent, Entities::ISkeletons)
