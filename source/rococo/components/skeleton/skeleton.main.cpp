#define ROCOCO_COMPONENTS_SKELETON_API __declspec(dllexport)

#include <components/rococo.components.skeleton.h>
#include <rococo.ecs.builder.inl>

namespace Rococo::Entities
{
	struct ISkeletons;
}

DEFINE_AND_EXPORT_SINGLETON_METHODS_WITH_LINKARG(ROCOCO_COMPONENTS_SKELETON_API, ISkeletonComponent, Entities::ISkeletons)



