#define ROCOCO_COMPONENTS_SKELETON_API __declspec(dllexport)
#define ROCOCO_COMPONENTS_API ROCOCO_COMPONENTS_SKELETON_API
#include <components/rococo.component.scripting.h>
#include <components/rococo.components.skeleton.h>
#include "../component.modules/skeleton/code-gen/skeleton.sxh.h"

namespace
{
	Rococo::Components::Generated::ISkeletonBase* FactoryConstructRococoComponentsSkeletonGetSkeleton(Rococo::Components::Generated::ISkeletonBase* base)
	{
		return base;
	}
}

#include "../component.modules/skeleton/code-gen/skeleton.sxh.inl"

PUBLISH_NATIVE_CALLS(Skeleton, AddNativeCalls_RococoComponentsGeneratedISkeletonBase)
