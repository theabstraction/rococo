#define ROCOCO_COMPONENTS_BODY_API __declspec(dllexport)

#include <components/rococo.components.body.h>
#include <rococo.ecs.builder.inl>

namespace Rococo::Components
{
	IComponentFactory<IBodyComponent>* CreateBodyFactory();
}

DEFINE_FACTORY_SINGLETON(IBodyComponent, CreateBodyFactory)

namespace Rococo::Components
{
	ROCOCO_COMPONENTS_BODY_API Ref<IBodyComponent> AddBodyComponent(ROID id)
	{
		return SINGLETON::AddComponent(id);
	}

	ROCOCO_COMPONENTS_BODY_API void BodyComponent_LinkToECS(IECS* ecs)
	{
		SINGLETON::GetTable().Link(ecs);
	}
}