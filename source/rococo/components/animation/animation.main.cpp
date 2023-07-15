#define ROCOCO_COMPONENTS_ANIMATION_API __declspec(dllexport)

#include <components/rococo.components.animation.h>
#include <rococo.ecs.builder.inl>

namespace Rococo::Components
{
	IComponentFactory<IAnimationComponent>* CreateAnimationFactory();
}

DEFINE_FACTORY_SINGLETON(IAnimationComponent, CreateAnimationFactory)

namespace Rococo::Components
{
	ROCOCO_COMPONENTS_ANIMATION_API Ref<IAnimationComponent> AddAnimationComponent(ROID id)
	{
		return SINGLETON::AddComponent(id);
	}

	ROCOCO_COMPONENTS_ANIMATION_API void AnimationComponent_LinkToECS(IECS* ecs)
	{
		SINGLETON::GetTable().Link(ecs);
	}
}