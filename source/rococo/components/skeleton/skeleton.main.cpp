#define ROCOCO_COMPONENTS_SKELETON_API __declspec(dllexport)

#include <components/rococo.components.skeleton.h>
#include <rococo.ecs.builder.inl>

namespace Rococo::Entities
{
	struct ISkeletons;
}

DEFINE_FACTORY_SINGLETON(ISkeletonComponent)
EXPORT_SINGLETON_METHODS_WITH_LINKARG(ROCOCO_COMPONENTS_SKELETON_API, ISkeletonComponent, Entities::ISkeletons)



