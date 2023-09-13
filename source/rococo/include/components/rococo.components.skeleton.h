#pragma once
#include <components/rococo.ecs.ex.h>
#include <rococo.meshes.h>
#include "../../component.modules/skeleton/code-gen/skeleton.sxh.h"

#ifndef ROCOCO_COMPONENTS_SKELETON_API
# define ROCOCO_COMPONENTS_SKELETON_API ROCOCO_API_IMPORT
#endif

namespace Rococo::Entities
{
    struct ISkeletons;
}

namespace Rococo::Components
{
    // This interfaces manages the association between a skeleton and a ROID.
    ROCOCO_INTERFACE ISkeletonComponent : Rococo::Components::Generated::ISkeletonBase
    {
        virtual Entities::ISkeleton* Skeleton() = 0;
        virtual void SetSkeleton(cstr skeletonName) = 0;
        virtual fstring SkeletonName() const = 0;
        virtual void SetFPSOrientation(const FPSAngles& orientation) = 0;
        virtual const FPSAngles& FPSOrientation() const = 0;
    };
}

DECLARE_SINGLETON_METHODS_WITH_LINK_ARG(ROCOCO_COMPONENTS_SKELETON_API, ISkeletonComponent, Entities::ISkeletons&)