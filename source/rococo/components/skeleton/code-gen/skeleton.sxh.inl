#pragma once

// Generated by BennyHill on 31/07/2023 19:25:49 UTC

#include <sexy.script.h>
#include "skeleton.sxh.h"

// BennyHill generated Sexy native functions for Rococo::Components::Skeleton::ISkeletonBase 
namespace
{
	using namespace Rococo;
	using namespace Rococo::Sex;
	using namespace Rococo::Script;
	using namespace Rococo::Compiler;


	void NativeGetHandleForRococoComponentsSkeletonGetSkeleton(NativeCallEnvironment& _nce)
	{
		Rococo::uint8* _sf = _nce.cpu.SF();
		ptrdiff_t _offset = 2 * sizeof(size_t);
		Rococo::Components::Skeleton::ISkeletonBase* nceContext = reinterpret_cast<Rococo::Components::Skeleton::ISkeletonBase*>(_nce.context);
		// Uses: Rococo::Components::Skeleton::ISkeletonBase* FactoryConstructRococoComponentsSkeletonGetSkeleton(Rococo::Components::Skeleton::ISkeletonBase* _context);
		Rococo::Components::Skeleton::ISkeletonBase* pObject = FactoryConstructRococoComponentsSkeletonGetSkeleton(nceContext);
		_offset += sizeof(IString*);
		WriteOutput(pObject, _sf, -_offset);
	}
}

namespace Rococo::Components::Skeleton
{
	void AddNativeCalls_RococoComponentsSkeletonISkeletonBase(Rococo::Script::IPublicScriptSystem& ss, Rococo::Components::Skeleton::ISkeletonBase* _nceContext)
	{
		HIDE_COMPILER_WARNINGS(_nceContext);
		const INamespace& ns = ss.AddNativeNamespace("Rococo.Components.Skeleton.Native");
		ss.AddNativeCall(ns, NativeGetHandleForRococoComponentsSkeletonGetSkeleton, _nceContext, ("GetHandleForISkeletonBase0  -> (Pointer hObject)"), __FILE__, __LINE__);
	}
}