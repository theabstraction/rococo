#pragma once
#include <rococo.types.h>

#include <components/rococo.components.animation.h>
#include <components/rococo.components.body.h>
#include <components/rococo.components.configuration.h>
#include <components/rococo.components.skeleton.h>

#define ROCOCO_INTEROP_IMPORT_API ROCOCO_API_IMPORT

namespace Rococo::Components::Generated::Interop
{
	ROCOCO_INTEROP_IMPORT_API void AddNativeCalls_ECS(Rococo::Script::IPublicScriptSystem& ss, Rococo::IECS* ecs);
	ROCOCO_INTEROP_IMPORT_API void AddNativeCalls_Animation(Rococo::Script::IPublicScriptSystem& ss);
	ROCOCO_INTEROP_IMPORT_API void AddNativeCalls_Body(Rococo::Script::IPublicScriptSystem& ss);
	ROCOCO_INTEROP_IMPORT_API void AddNativeCalls_Configuration(Rococo::Script::IPublicScriptSystem& ss);
	ROCOCO_INTEROP_IMPORT_API void AddNativeCalls_Skeleton(Rococo::Script::IPublicScriptSystem& ss);

	void AddComponentNatives(IPublicScriptSystem& ss, Rococo::IECS* ecs)
	{
		Rococo::Components::Generated::Interop::AddNativeCalls_ECS(ss, ecs);
		Rococo::Components::Generated::Interop::AddNativeCalls_Animation(ss);
		Rococo::Components::Generated::Interop::AddNativeCalls_Body(ss);
		Rococo::Components::Generated::Interop::AddNativeCalls_Configuration(ss);
		Rococo::Components::Generated::Interop::AddNativeCalls_Skeleton(ss);
	}
}