#include <components/rococo.components.body.h>
#include <new>

namespace Rococo::Components
{
	struct BodyComponent : IBodyComponent
	{		
	};

	IComponentFactory<IBodyComponent>* CreateBodyFactory()
	{
		return new DefaultFactory<IBodyComponent, BodyComponent>();
	}
}