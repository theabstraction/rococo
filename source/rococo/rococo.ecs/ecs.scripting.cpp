#include <rococo.ecs.h>
#include <sexy.script.h>

namespace
{
	Rococo::IECSBase* FactoryConstructRococoECSGetECS(Rococo::IECS* _context)
	{
		return _context;
	}
}

#include <../rococo.ecs/code-gen/rococo.ecs.sxh.inl>

namespace Rococo::ECS
{
	ROCOCO_ECS_API void AddNativeCalls(Rococo::Script::IPublicScriptSystem& ss, Rococo::IECS* ecs)
	{
		Rococo::AddNativeCalls_RococoIECSBase(ss, ecs);
	}
}