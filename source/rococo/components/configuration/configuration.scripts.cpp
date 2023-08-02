#define ROCOCO_COMPONENTS_CONFIG_API __declspec(dllexport)

#include <rococo.ecs.h>
#include <sexy.script.h>
#include <components/rococo.components.configuration.h>

#include <..\components\configuration\code-gen\config.sxh.h>

namespace
{
	Rococo::Components::Generated::IConfigBase* FactoryConstructRococoComponentsConfigGetConfig(Rococo::Components::Generated::IConfigBase* config)
	{
		return config;
	}
}

#include <..\components\configuration\code-gen\config.sxh.inl>

namespace Rococo::Components::Config
{
	ROCOCO_COMPONENTS_CONFIG_API void AddNativeCalls(Rococo::Script::IPublicScriptSystem& ss, Rococo::Components::Generated::IConfigBase* config)
	{
		AddNativeCalls_RococoComponentsGeneratedIConfigBase(ss, config);
	}
}