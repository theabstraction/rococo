#define ROCOCO_COMPONENTS_SKELETON_API __declspec(dllexport)

#include <rococo.ecs.h>
#include <sexy.script.h>

#include <..\components\skeleton\code-gen\skeleton.sxh.h>

namespace
{
	Rococo::Components::Skeleton::ISkeletonBase* FactoryConstructRococoComponentsSkeletonGetSkeleton(Rococo::Components::Skeleton::ISkeletonBase* base)
	{
		return base;
	}
}

#include <..\components\skeleton\code-gen\skeleton.sxh.inl>

namespace Rococo::Components::Skeleton
{
	ROCOCO_COMPONENTS_SKELETON_API void AddNativeCalls(Rococo::Script::IPublicScriptSystem& ss, Rococo::Components::Skeleton::ISkeletonBase* base)
	{
		AddNativeCalls_RococoComponentsSkeletonISkeletonBase(ss, base);
	}
}