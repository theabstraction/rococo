#pragma once

namespace Rococo::Entities
{
	struct AnimationAdvanceArgs
	{
		ISkeleton& puppet;
		ISkeletons& poses;
		const Seconds dt;
	};

	ROCOCOAPI IAnimation
	{
		virtual	void AddKeyFrame(const fstring & frameName, Seconds duration, boolean32 loop) = 0;
		virtual void Advance(AnimationAdvanceArgs& args) = 0;
		virtual void Free() = 0;
	};

	IAnimation* CreateAnimation();
}