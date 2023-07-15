#define ROCOCO_COMPONENTS_SKELETON_API __declspec(dllexport)

#include <components/rococo.components.skeleton.h>
#include <rococo.ecs.builder.inl>

namespace Rococo::Components
{
	IComponentFactory<ISkeletonComponent>* CreateSkeletonFactory();
}

DEFINE_FACTORY_SINGLETON(ISkeletonComponent, CreateSkeletonFactory)

namespace Rococo::Components
{
	ROCOCO_COMPONENTS_SKELETON_API Ref<ISkeletonComponent> AddSkeletonComponent(ROID id)
	{
		return SINGLETON::AddComponent(id);
	}

	ROCOCO_COMPONENTS_SKELETON_API void SkeletonComponent_LinkToECS(IECS* ecs)
	{
		SINGLETON::GetTable().Link(ecs);
	}
}