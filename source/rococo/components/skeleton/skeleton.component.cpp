#define ROCOCO_COMPONENTS_SKELETON_API __declspec(dllexport)

#include <components/rococo.components.skeleton.h>
#include <rococo.maths.h>
#include <rococo.strings.h>
#include <rococo.animation.h>
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

    static ISkeletons* s_Skeletons = nullptr;

    IComponentFactory<ISkeletonComponent>* CreateSkeletonFactory()
    {
        if (s_Skeletons == nullptr)
        {
            Throw(0, "Skeletons have not been assigned. Ensure Rococo::Components::SkeletonComponent_Init(...) is called before other functions of the Skeleton component API");
        }
        return new FactoryWithOneArg<ISkeletonComponent, SkeletonComponent, ISkeletons>(*s_Skeletons);
    }
}

namespace Rococo::Components
{
    void SkeletonComponent_Init(ISkeletons& skeletons)
    {
        if (s_Skeletons != nullptr)
        {
            Throw(0, "%s: Skeletons already assigned", __FUNCTION__);
        }
        else
        {
            s_Skeletons = &skeletons;
        }
    }
}