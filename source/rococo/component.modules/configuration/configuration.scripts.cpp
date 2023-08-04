#define ROCOCO_COMPONENTS_CONFIG_API __declspec(dllexport)
#define ROCOCO_COMPONENTS_API ROCOCO_COMPONENTS_CONFIG_API
#include <components/rococo.component.scripting.h>
#include <components/rococo.components.configuration.h>
#include "../../component.modules/configuration/code-gen/config.sxh.h"

namespace
{
	Rococo::Components::Generated::IConfigBase* FactoryConstructRococoComponentsConfigGetConfig(Rococo::Components::Generated::IConfigBase* config)
	{
		return config;
	}
}

#include "../../component.modules/configuration/code-gen/config.sxh.inl"

PUBLISH_NATIVE_CALLS(Configuration, AddNativeCalls_RococoComponentsGeneratedIConfigBase)