#define ROCOCO_COMPONENTS_CONFIG_API __declspec(dllexport)

#include <components/rococo.components.configuration.h>
#include <rococo.ecs.builder.inl>

namespace Rococo::Components
{
	IComponentFactory<IConfigurationComponent>* CreateConfigurationFactory();
}

DEFINE_FACTORY_SINGLETON(IConfigurationComponent, CreateConfigurationFactory)

namespace Rococo::Components
{
	ROCOCO_COMPONENTS_CONFIG_API Ref<IConfigurationComponent> AddConfigurationComponent(ROID id)
	{
		return SINGLETON::AddComponent(id);
	}

	ROCOCO_COMPONENTS_CONFIG_API void ConfigurationComponent_LinkToECS(IECS* ecs)
	{
		SINGLETON::GetTable().Link(ecs);
	}
}