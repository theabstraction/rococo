#define ROCOCO_COMPONENTS_BODY_API __declspec(dllexport)

#include <rococo.ecs.h>
#include <sexy.script.h>

#include <components/rococo.components.body.h>

using namespace Rococo::Components::Generated;

namespace
{
	IBodyBase* FactoryConstructRococoComponentsGeneratedGetBody(IBodyBase* base)
	{
		return base;
	}
}

#include <..\components\body\code-gen\body.sxh.inl>

namespace Rococo::Components::Body
{
	ROCOCO_COMPONENTS_BODY_API void AddNativeCalls(Rococo::Script::IPublicScriptSystem& ss)
	{
		Rococo::Components::Generated::Interop::AddNativeCalls_RococoComponentsGeneratedIBodyBase(ss, nullptr);
	}
}
