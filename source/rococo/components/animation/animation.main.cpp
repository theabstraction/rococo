#define ROCOCO_COMPONENTS_ANIMATION_API __declspec(dllexport)

#include <components/rococo.components.animation.h>
#include <rococo.ecs.builder.inl>

namespace Rococo::Components
{
	IComponentFactory<IAnimationComponent>* CreateAnimationFactory();
}

DEFINE_FACTORY_SINGLETON(IAnimationComponent, CreateAnimationFactory)
EXPORT_SINGLETON_METHODS(ROCOCO_COMPONENTS_ANIMATION_API, IAnimationComponent)