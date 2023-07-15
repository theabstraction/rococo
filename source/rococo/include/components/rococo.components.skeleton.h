#pragma once
#include <rococo.ecs.h>
#include <rococo.meshes.h>

#ifndef ROCOCO_COMPONENTS_SKELETON_API
# define ROCOCO_COMPONENTS_SKELETON_API ROCOCO_API_IMPORT
#endif

namespace Rococo::Entities
{
    struct ISkeletons;
}

namespace Rococo::Components
{
    ROCOCO_INTERFACE ISkeletonComponent : IComponentBase
    {
        virtual Entities::ISkeleton* Skeleton() = 0;
        virtual void SetSkeleton(cstr skeletonName) = 0;
        virtual fstring SkeletonName() const = 0;
        virtual void SetFPSOrientation(const FPSAngles& orientation) = 0;
        virtual const FPSAngles& FPSOrientation() const = 0;
    };

    ROCOCO_COMPONENTS_SKELETON_API Ref<ISkeletonComponent> AddSkeletonComponent(ROID id);
    ROCOCO_COMPONENTS_SKELETON_API Ref<ISkeletonComponent> GetSkeletonComponent(ROID id);
    ROCOCO_COMPONENTS_SKELETON_API void SkeletonComponent_LinkToECS(IECS& ecs, Entities::ISkeletons& skeletons);
}