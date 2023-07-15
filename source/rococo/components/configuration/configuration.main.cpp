#define ROCOCO_COMPONENTS_CONFIG_API __declspec(dllexport)

#include <components/rococo.components.configuration.h>
#include <rococo.ecs.builder.inl>

namespace Rococo::Components
{
	IComponentFactory<IConfigurationComponent>* CreateConfigurationFactory();
}

DEFINE_FACTORY_SINGLETON(IConfigurationComponent, CreateConfigurationFactory)
EXPORT_SINGLETON_METHODS(ROCOCO_COMPONENTS_CONFIG_API, IConfigurationComponent)