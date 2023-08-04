#pragma once
#ifndef ROCOCO_COMPONENTS_API
static_assert(false, "Define ROCOCO_COMPONENTS_API before including this file. ROCOCO_COMPONENTS_API should be set to ROCOCO_API_EXPORT or ROCOCO_API_IMPORT or blank")
#endif

#include <components/rococo.ecs.h>
#include <components/rococo.component.base.interop.h>
#include <sexy.script.h>

#define PUBLISH_NATIVE_CALLS(SHORTNAME, INTERNAL_ADD_NATIVE_CALL)										\
namespace Rococo::Components::Generated::Interop														\
{																										\
	ROCOCO_COMPONENTS_API void AddNativeCalls_##SHORTNAME(Rococo::Script::IPublicScriptSystem& ss)		\
	{																									\
		Rococo::Components::Generated::Interop::##INTERNAL_ADD_NATIVE_CALL(ss, nullptr);				\
	}																									\
}

