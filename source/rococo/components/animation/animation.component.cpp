#define ROCOCO_COMPONENTS_ANIMATION_API __declspec(dllexport)
#include <components/rococo.components.animation.h>
#include <rococo.animation.h>
#include <rococo.ecs.builder.inl>
#include <new>

namespace Rococo::Components
{
	using namespace Rococo::Entities;

	struct AnimationComponent : IAnimationComponent
	{
		AutoFree<IAnimation> animation;
		AnimationComponent():
			animation(CreateAnimation())
		{

		}

		Rococo::Entities::IAnimation& Core() override
		{
			return *animation;
		}
	};

	namespace API::ForIAnimationComponent
	{
		IComponentFactory<IAnimationComponent>* CreateComponentFactory()
		{
			return new DefaultFactory<IAnimationComponent, AnimationComponent>();
		}
	}
}

DEFINE_AND_EXPORT_SINGLETON_METHODS_WITH_DEFAULT_FACTORY(ROCOCO_COMPONENTS_ANIMATION_API, IAnimationComponent, AnimationComponent)