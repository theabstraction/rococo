#define ROCOCO_COMPONENTS_SKELETON_API __declspec(dllexport)

#include <components/rococo.components.skeleton.h>
#include <rococo.ecs.builder.inl>

namespace Rococo::Components
{
	IComponentFactory<ISkeletonComponent>* CreateSkeletonFactory();
	void SkeletonComponent_Init(Entities::ISkeletons& skeletons);
}

namespace Rococo::Entities
{
	struct ISkeletons;
}

DEFINE_FACTORY_SINGLETON(ISkeletonComponent, CreateSkeletonFactory)
EXPORT_SINGLETON_METHODS_FOR_LINKARG(ROCOCO_COMPONENTS_SKELETON_API, ISkeletonComponent, Entities::ISkeletons)