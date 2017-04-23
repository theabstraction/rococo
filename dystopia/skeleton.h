#ifndef DYSTOPIA_SKELETON
#define DYSTOPIA_SKELETON

namespace Dystopia
{
	struct BoneOrientation
	{
		Quat rotation;
		Vec3 parentToChildDisplacement;
		float unused;
	};

	ROCOCO_ID(ID_KEYFRAME, uint32, 0);
	ROCOCO_ID(ID_ANIMATION, uint32, 0);

	ROCOCOAPI ISkeleton
	{
		// TODO - virtual void SetNextAnimation(AnimationType type) = 0;
		virtual void SetCurrentAnimation(AnimationType type) = 0;
	};

	ROCOCOAPI ISkeletonSupervisor: public ISkeleton
	{
		virtual void Free() = 0;
		virtual void Render(IRenderContext& rc, const ObjectInstance& instance, Seconds gameTime) = 0;
	};

	ISkeletonSupervisor* CreateSkeleton(Environment& e);

	ROCOCOAPI IBoneEnumerator
	{
		// Called once for each bone in the skeleton
		virtual void OnBone(const BoneOrientation& bone, int32 index) = 0;

		// Called prior to OnBone to tell the enumerator what type of skeleton we have
		virtual void OnType(SkeletonType type) = 0;
	};

	struct FrameInfo
	{
		ID_KEYFRAME id;
		
		// if id is invalid, the nextAnimationId gives follow up animation
		ID_ANIMATION nextAnimationId; 

		// The time after the start of the animation sequence that this keyframe becomes the current state
		Seconds actualisationTime;
	};

	struct AnimationSequence
	{
		const FrameInfo* firstFrame;
		const FrameInfo* lastFrame_plus_one;
		const size_t NumberOfKeyframes;
		const ID_ANIMATION id;
		const FrameInfo* begin() const { return firstFrame; }
		const FrameInfo* end() const { return lastFrame_plus_one; }
	};

	ROCOCOAPI IBoneLibrary
	{
		virtual AnimationSequence GetAnimationSequenceByName(cstr name) = 0;
		virtual AnimationSequence GetAnimationSequenceById(ID_ANIMATION id) = 0;
		virtual int32 EnumerateKeyframeBonesByName(cstr name, IBoneEnumerator& onFrame) = 0;
		virtual int32 EnumerateKeyframeBonesById(ID_KEYFRAME id, IBoneEnumerator& onFrame) = 0;
		virtual void UpdateLib(cstr filename) = 0;
	};

	ROCOCOAPI IBoneLibrarySupervisor: public IBoneLibrary
	{
		virtual void Free() = 0;
		virtual void Reload(cstr libraryName) = 0;
	};

	IBoneLibrarySupervisor* CreateBoneLibrary(IInstallation& installation, IRenderer& renderer, ISourceCache& sources);
}

#endif
