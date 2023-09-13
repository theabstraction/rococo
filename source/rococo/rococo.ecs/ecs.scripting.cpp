#define ROCOCO_COMPONENTS_API ROCOCO_ECS_API
#include <components/rococo.component.scripting.h>

namespace
{
	Rococo::IECSBase* FactoryConstructRococoECSGetECS(Rococo::IECS* _context)
	{
		return _context;
	}
}

#include "../rococo.ecs/code-gen/ecs.sxh.inl"

namespace Rococo::Components::Generated::Interop
{
	ROCOCO_ECS_API void AddNativeCalls_ECS(Rococo::Script::IPublicScriptSystem& ss, Rococo::IECS* ecs)
	{
		Rococo::Interop::AddNativeCalls_RococoIECSBase(ss, ecs);
	}
}