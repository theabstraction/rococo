#define ROCOCO_COMPONENTS_BODY_API __declspec(dllexport)

#include <rococo.ecs.h>
#include <sexy.script.h>

#include <components/rococo.components.body.h>

#include <..\components\body\code-gen\body.sxh.h>

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
		AddNativeCalls_RococoComponentsGeneratedIBodyBase(ss, nullptr);
	}
}

/*
(namespace Rococo.Components.Body)
(using Rococo.Components.Body)

(interface Rococo.Components.Body.IBodyBase
)

(class ProxyIBodyBase
	(implements IBodyBase)
	(attribute not- serialized)
	(Sys.Type.ComponentRef ref)
	(Rococo.Components.ROID roid)
)

(method ProxyIBodyBase.Construct(ROID id)(Sys.Type.ComponentRef ref) :
	this.roid = id;
	this.ref.hComponent = ref.hComponent;
	this.ref.hLife = ref.hLife;
)

(factory Rococo.Components.Body.AddBody IBodyBase (Rococo.Components.ROID id) :
	(Sys.Type.ComponentRef ref)
	(Native.AddBody ref id)

	(if (outref.hComponent == 0)
		(return)
	)
	(construct ProxyIBodyBase id ref)
)

(factory Rococo.Components.Body.GetBody IBodyBase (Rococo.Components.ROID id):
	(Sys.Type.ComponentRef ref)
	(Native.GetBody ref id)

	(if (outref.hComponent == 0)
		(return)
	)

	(construct ProxyIBodyBase id ref)
)
*/