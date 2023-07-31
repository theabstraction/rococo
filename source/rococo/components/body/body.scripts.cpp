#define ROCOCO_COMPONENTS_BODY_API __declspec(dllexport)

#include <rococo.ecs.h>
#include <sexy.script.h>

#include <..\components\body\code-gen\body.sxh.h>

namespace
{
	Rococo::Components::Body::IBodyBase* FactoryConstructRococoComponentsBodyGetBody(Rococo::Components::Body::IBodyBase* base)
	{
		return base;
	}
}

#include <..\components\body\code-gen\body.sxh.inl>

namespace Rococo::Components::Body
{
	ROCOCO_COMPONENTS_BODY_API void AddNativeCalls(Rococo::Script::IPublicScriptSystem& ss, Rococo::Components::Body::IBodyBase* base)
	{
		AddNativeCalls_RococoComponentsBodyIBodyBase(ss, base);
	}
}