#define ROCOCO_COMPONENTS_BODY_API __declspec(dllexport)

#include <components/rococo.components.body.h>
#include <rococo.ecs.builder.inl>

DEFINE_FACTORY_SINGLETON(IBodyComponent)
EXPORT_SINGLETON_METHODS(ROCOCO_COMPONENTS_BODY_API, IBodyComponent);