#define ROCOCO_COMPONENTS_API __declspec(dllexport)
#define ROCOCO_COMPONENTS_BODY_API ROCOCO_COMPONENTS_API
#include <components/rococo.component.scripting.h>
#include <components/rococo.components.body.h>

using namespace Rococo::Components::Generated;

namespace
{
	IBodyBase* FactoryConstructRococoComponentsGeneratedGetBody(IBodyBase* base)
	{
		return base;
	}
}

#include <..\component.modules\body\code-gen\body.sxh.inl>

PUBLISH_NATIVE_CALLS(Body, AddNativeCalls_RococoComponentsGeneratedIBodyBase)
