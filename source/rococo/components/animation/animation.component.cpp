#include <components/rococo.components.animation.h>
#include <rococo.animation.h>
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

	IComponentFactory<IAnimationComponent>* CreateAnimationFactory()
	{
		return new DefaultFactory<IAnimationComponent, AnimationComponent>();
	}
}