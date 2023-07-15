#define ROCOCO_COMPONENTS_CONFIG_API __declspec(dllexport)

#include <components/rococo.components.configuration.h>
#include <rococo.ecs.builder.inl>

DEFINE_AND_EXPORT_SINGLETON_METHODS(ROCOCO_COMPONENTS_CONFIG_API, IConfigurationComponent)
